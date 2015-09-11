/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
 *
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include <sstream>

#include "mongo/platform/basic.h"

#include "rocks_server_status.h"

#include "boost/scoped_ptr.hpp"

#include <rocksdb/db.h>
#include <rocksdb/env.h>
#include <rocksdb/thread_status.h>

#include "mongo/base/checked_cast.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/scopeguard.h"

#include "rocks_recovery_unit.h"
#include "rocks_engine.h"
#include "rocks_transaction.h"

namespace mongo {
using std::string;

namespace {
std::string PrettyPrintBytes(size_t bytes) {
    if (bytes < (16 << 10)) {
        return std::to_string(bytes) + "B";
    } else if (bytes < (16 << 20)) {
        return std::to_string(bytes >> 10) + "KB";
    } else if (bytes < (16LL << 30)) {
        return std::to_string(bytes >> 20) + "MB";
    } else {
        return std::to_string(bytes >> 30) + "GB";
    }
}
}  // namespace

RocksServerStatusSection::RocksServerStatusSection(RocksEngine* engine)
    : ServerStatusSection("rocksdb"), _engine(engine) {}

bool RocksServerStatusSection::includeByDefault() const {
    return true;
}

BSONObj RocksServerStatusSection::generateSection(OperationContext* txn,
                                                  const BSONElement& configElement) const {
    BSONObjBuilder bob;

    // if the second is true, that means that we pass the value through PrettyPrintBytes
    std::vector<std::pair<std::string, bool>> properties = {{"stats", false},
                                                            {"num-immutable-mem-table", false},
                                                            {"mem-table-flush-pending", false},
                                                            {"compaction-pending", false},
                                                            {"background-errors", false},
                                                            {"cur-size-active-mem-table", true},
                                                            {"cur-size-all-mem-tables", true},
                                                            {"num-entries-active-mem-table", false},
                                                            {"num-entries-imm-mem-tables", false},
                                                            {"estimate-table-readers-mem", true},
                                                            {"num-snapshots", false},
                                                            {"oldest-snapshot-time", false},
                                                            {"num-live-versions", false}};
    for (auto const& property : properties) {
        std::string statsString;
        if (!_engine->getDB()->GetProperty("rocksdb." + property.first, &statsString)) {
            statsString = "<error> unable to retrieve statistics";
        }
        if (property.first == "stats") {
            // special casing because we want to turn it into array
            BSONArrayBuilder a;
            std::stringstream ss(statsString);
            std::string line;
            while (std::getline(ss, line)) {
                a.append(line);
            }
            bob.appendArray(property.first, a.arr());
        } else if (property.second) {
            bob.append(property.first, PrettyPrintBytes(std::stoll(statsString)));
        } else {
            bob.append(property.first, statsString);
        }
    }
    bob.append("total-live-recovery-units", RocksRecoveryUnit::getTotalLiveRecoveryUnits());
    bob.append("block-cache-usage", PrettyPrintBytes(_engine->getBlockCacheUsage()));
    bob.append("transaction-engine-keys",
               static_cast<long long>(_engine->getTransactionEngine()->numKeysTracked()));
    bob.append("transaction-engine-snapshots",
               static_cast<long long>(_engine->getTransactionEngine()->numActiveSnapshots()));

    std::vector<rocksdb::ThreadStatus> threadList;
    auto s = rocksdb::Env::Default()->GetThreadList(&threadList);
    if (s.ok()) {
        BSONArrayBuilder threadStatus;
        for (auto& ts : threadList) {
            if (ts.operation_type == rocksdb::ThreadStatus::OP_UNKNOWN ||
                ts.thread_type == rocksdb::ThreadStatus::USER) {
                continue;
            }
            BSONObjBuilder threadObjBuilder;
            threadObjBuilder.append("type",
                                    rocksdb::ThreadStatus::GetOperationName(ts.operation_type));
            threadObjBuilder.append("time_elapsed",
                                    rocksdb::ThreadStatus::MicrosToString(ts.op_elapsed_micros));
            auto op_properties = rocksdb::ThreadStatus::InterpretOperationProperties(
                ts.operation_type, ts.op_properties);

            const std::vector<std::pair<std::string, std::string>> properties(
                {{"JobID", "job_id"},
                 {"BaseInputLevel", "input_level"},
                 {"OutputLevel", "output_level"},
                 {"IsManual", "manual"}});
            for (const auto& prop : properties) {
                auto itr = op_properties.find(prop.first);
                if (itr != op_properties.end()) {
                    threadObjBuilder.append(prop.second, static_cast<int>(itr->second));
                }
            }

            const std::vector<std::pair<std::string, std::string>> byte_properties(
                {{"BytesRead", "bytes_read"},
                 {"BytesWritten", "bytes_written"},
                 {"TotalInputBytes", "total_bytes"}});
            for (const auto& prop : byte_properties) {
                auto itr = op_properties.find(prop.first);
                if (itr != op_properties.end()) {
                    threadObjBuilder.append(prop.second,
                                            PrettyPrintBytes(static_cast<size_t>(itr->second)));
                }
            }

            const std::vector<std::pair<std::string, std::string>> speed_properties(
                {{"BytesRead", "read_throughput"}, {"BytesWritten", "write_throughput"}});
            for (const auto& prop : speed_properties) {
                auto itr = op_properties.find(prop.first);
                if (itr != op_properties.end()) {
                    size_t speed =
                        (itr->second * 1000 * 1000) / static_cast<size_t>(ts.op_elapsed_micros);
                    threadObjBuilder.append(prop.second,
                                            PrettyPrintBytes(static_cast<size_t>(speed)) + "/s");
                }
            }

            threadStatus.append(threadObjBuilder.obj());
        }
        bob.appendArray("thread-status", threadStatus.arr());
    }

    return bob.obj();
}

}  // namespace mongo
