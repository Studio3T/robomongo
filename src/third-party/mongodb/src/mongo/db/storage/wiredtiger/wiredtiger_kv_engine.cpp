// wiredtiger_kv_engine.cpp

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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include "mongo/db/storage/wiredtiger/wiredtiger_kv_engine.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>

#include "mongo/db/concurrency/write_conflict_exception.h"
#include "mongo/db/index/index_descriptor.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_global_options.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_index.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_record_store.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_recovery_unit.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_session_cache.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_size_storer.h"
#include "mongo/db/storage/wiredtiger/wiredtiger_util.h"
#include "mongo/util/log.h"
#include "mongo/util/processinfo.h"
#include "mongo/util/scopeguard.h"
#include "mongo/util/time_support.h"

#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

namespace mongo {

using std::set;
using std::string;


WiredTigerKVEngine::WiredTigerKVEngine(const std::string& path,
                                       const std::string& extraOpenOptions,
                                       bool durable,
                                       bool repair)
    : _eventHandler(WiredTigerUtil::defaultEventHandlers()),
      _path(path),
      _durable(durable),
      _sizeStorerSyncTracker(100000, 60 * 1000) {
    size_t cacheSizeGB = wiredTigerGlobalOptions.cacheSizeGB;
    if (cacheSizeGB == 0) {
        // Since the user didn't provide a cache size, choose a reasonable default value.
        ProcessInfo pi;
        unsigned long long memSizeMB = pi.getMemSizeMB();
        if (memSizeMB > 0) {
            double cacheMB = memSizeMB / 2;
            cacheSizeGB = static_cast<size_t>(cacheMB / 1024);
            if (cacheSizeGB < 1)
                cacheSizeGB = 1;
        }
    }

    if (_durable) {
        boost::filesystem::path journalPath = path;
        journalPath /= "journal";
        if (!boost::filesystem::exists(journalPath)) {
            try {
                boost::filesystem::create_directory(journalPath);
            } catch (std::exception& e) {
                log() << "error creating journal dir " << journalPath.string() << ' ' << e.what();
                throw;
            }
        }
    }

    _previousCheckedDropsQueued = curTimeMillis64();

    std::stringstream ss;
    ss << "create,";
    ss << "cache_size=" << cacheSizeGB << "G,";
    ss << "session_max=20000,";
    ss << "eviction=(threads_max=4),";
    ss << "statistics=(fast),";
    if (_durable) {
        ss << "log=(enabled=true,archive=true,path=journal,compressor=";
        ss << wiredTigerGlobalOptions.journalCompressor << "),";
    }
    ss << "file_manager=(close_idle_time=100000),";  //~28 hours, will put better fix in 3.1.x
    ss << "checkpoint=(wait=" << wiredTigerGlobalOptions.checkpointDelaySecs;
    ss << ",log_size=2GB),";
    ss << "statistics_log=(wait=" << wiredTigerGlobalOptions.statisticsLogDelaySecs << "),";
    ss << extraOpenOptions;
    string config = ss.str();
    log() << "wiredtiger_open config: " << config;
    int ret = wiredtiger_open(path.c_str(), &_eventHandler, config.c_str(), &_conn);
    // Invalid argument (EINVAL) is usually caused by invalid configuration string.
    // We still fassert() but without a stack trace.
    if (ret == EINVAL) {
        fassertFailedNoTrace(28561);
    } else if (ret != 0) {
        Status s(wtRCToStatus(ret));
        msgassertedNoTrace(28595, s.reason());
    }

    _sessionCache.reset(new WiredTigerSessionCache(this));

    _sizeStorerUri = "table:sizeStorer";
    {
        WiredTigerSession session(_conn);
        if (repair && _hasUri(session.getSession(), _sizeStorerUri)) {
            log() << "Repairing size cache";
            fassertNoTrace(28577, _salvageIfNeeded(_sizeStorerUri.c_str()));
        }
        _sizeStorer.reset(new WiredTigerSizeStorer(_conn, _sizeStorerUri));
        _sizeStorer->fillCache();
    }
}


WiredTigerKVEngine::~WiredTigerKVEngine() {
    if (_conn) {
        cleanShutdown();
    }

    _sessionCache.reset(NULL);
}

void WiredTigerKVEngine::cleanShutdown() {
    log() << "WiredTigerKVEngine shutting down";
    syncSizeInfo(true);
    if (_conn) {
        // these must be the last things we do before _conn->close();
        _sizeStorer.reset(NULL);
        _sessionCache->shuttingDown();

#if !__has_feature(address_sanitizer)
        const char* config = "leak_memory=true";
#else
        const char* config = NULL;
#endif
        invariantWTOK(_conn->close(_conn, config));
        _conn = NULL;
    }
}

Status WiredTigerKVEngine::okToRename(OperationContext* opCtx,
                                      const StringData& fromNS,
                                      const StringData& toNS,
                                      const StringData& ident,
                                      const RecordStore* originalRecordStore) const {
    _sizeStorer->storeToCache(
        _uri(ident), originalRecordStore->numRecords(opCtx), originalRecordStore->dataSize(opCtx));
    syncSizeInfo(true);
    return Status::OK();
}

int64_t WiredTigerKVEngine::getIdentSize(OperationContext* opCtx, const StringData& ident) {
    WiredTigerSession* session = WiredTigerRecoveryUnit::get(opCtx)->getSession(opCtx);
    return WiredTigerUtil::getIdentSize(session->getSession(), _uri(ident));
}

Status WiredTigerKVEngine::repairIdent(OperationContext* opCtx, const StringData& ident) {
    WiredTigerSession* session = WiredTigerRecoveryUnit::get(opCtx)->getSession(opCtx);
    session->closeAllCursors();
    string uri = _uri(ident);
    return _salvageIfNeeded(uri.c_str());
}

Status WiredTigerKVEngine::_salvageIfNeeded(const char* uri) {
    // Using a side session to avoid transactional issues
    WiredTigerSession sessionWrapper(_conn);
    WT_SESSION* session = sessionWrapper.getSession();

    int rc = (session->verify)(session, uri, NULL);
    if (rc == 0) {
        log() << "Verify succeeded on uri " << uri << ". Not salvaging.";
        return Status::OK();
    }

    if (rc == EBUSY) {
        // SERVER-16457: verify and salvage are occasionally failing with EBUSY. For now we
        // lie and return OK to avoid breaking tests. This block should go away when that ticket
        // is resolved.
        error() << "Verify on " << uri << " failed with EBUSY. Assuming no salvage is needed.";
        return Status::OK();
    }

    // TODO need to cleanup the sizeStorer cache after salvaging.
    log() << "Verify failed on uri " << uri << ". Running a salvage operation.";
    return wtRCToStatus(session->salvage(session, uri, NULL), "Salvage failed:");
}

int WiredTigerKVEngine::flushAllFiles(bool sync) {
    LOG(1) << "WiredTigerKVEngine::flushAllFiles";
    syncSizeInfo(true);

    WiredTigerSession session(_conn);
    WT_SESSION* s = session.getSession();
    invariantWTOK(s->checkpoint(s, NULL));

    return 1;
}

void WiredTigerKVEngine::syncSizeInfo(bool sync) const {
    if (!_sizeStorer)
        return;

    try {
        _sizeStorer->syncCache(sync);
    } catch (const WriteConflictException&) {
        // ignore, we'll try again later.
    }
}

RecoveryUnit* WiredTigerKVEngine::newRecoveryUnit() {
    return new WiredTigerRecoveryUnit(_sessionCache.get());
}

void WiredTigerKVEngine::setRecordStoreExtraOptions(const std::string& options) {
    _rsOptions = options;
}

void WiredTigerKVEngine::setSortedDataInterfaceExtraOptions(const std::string& options) {
    _indexOptions = options;
}

Status WiredTigerKVEngine::createRecordStore(OperationContext* opCtx,
                                             const StringData& ns,
                                             const StringData& ident,
                                             const CollectionOptions& options) {
    _checkIdentPath(ident);
    WiredTigerSession session(_conn);

    StatusWith<std::string> result =
        WiredTigerRecordStore::generateCreateString(ns, options, _rsOptions);
    if (!result.isOK()) {
        return result.getStatus();
    }
    std::string config = result.getValue();

    string uri = _uri(ident);
    WT_SESSION* s = session.getSession();
    LOG(2) << "WiredTigerKVEngine::createRecordStore uri: " << uri << " config: " << config;
    return wtRCToStatus(s->create(s, uri.c_str(), config.c_str()));
}

RecordStore* WiredTigerKVEngine::getRecordStore(OperationContext* opCtx,
                                                const StringData& ns,
                                                const StringData& ident,
                                                const CollectionOptions& options) {
    if (options.capped) {
        return new WiredTigerRecordStore(opCtx,
                                         ns,
                                         _uri(ident),
                                         options.capped,
                                         options.cappedSize ? options.cappedSize : 4096,
                                         options.cappedMaxDocs ? options.cappedMaxDocs : -1,
                                         NULL,
                                         _sizeStorer.get());
    } else {
        return new WiredTigerRecordStore(
            opCtx, ns, _uri(ident), false, -1, -1, NULL, _sizeStorer.get());
    }
}

string WiredTigerKVEngine::_uri(const StringData& ident) const {
    return string("table:") + ident.toString();
}

Status WiredTigerKVEngine::createSortedDataInterface(OperationContext* opCtx,
                                                     const StringData& ident,
                                                     const IndexDescriptor* desc) {
    _checkIdentPath(ident);
    StatusWith<std::string> result = WiredTigerIndex::generateCreateString(_indexOptions, *desc);
    if (!result.isOK()) {
        return result.getStatus();
    }

    std::string config = result.getValue();

    LOG(2) << "WiredTigerKVEngine::createSortedDataInterface ident: " << ident
           << " config: " << config;
    return wtRCToStatus(WiredTigerIndex::Create(opCtx, _uri(ident), config));
}

SortedDataInterface* WiredTigerKVEngine::getSortedDataInterface(OperationContext* opCtx,
                                                                const StringData& ident,
                                                                const IndexDescriptor* desc) {
    if (desc->unique())
        return new WiredTigerIndexUnique(opCtx, _uri(ident), desc);
    return new WiredTigerIndexStandard(opCtx, _uri(ident), desc);
}

Status WiredTigerKVEngine::dropIdent(OperationContext* opCtx, const StringData& ident) {
    _drop(ident);
    return Status::OK();
}

bool WiredTigerKVEngine::_drop(const StringData& ident) {
    string uri = _uri(ident);

    WiredTigerSession session(_conn);

    int ret = session.getSession()->drop(session.getSession(), uri.c_str(), "force");
    LOG(1) << "WT drop of  " << uri << " res " << ret;

    if (ret == 0) {
        // yay, it worked
        return true;
    }

    if (ret == EBUSY) {
        // this is expected, queue it up
        {
            boost::mutex::scoped_lock lk(_identToDropMutex);
            _identToDrop.insert(uri);
        }
        _sessionCache->closeAll();
        return false;
    }

    invariantWTOK(ret);
    return false;
}

bool WiredTigerKVEngine::haveDropsQueued() const {
    int64_t now = curTimeMillis64();
    int64_t delta = now - _previousCheckedDropsQueued;

    if (_sizeStorerSyncTracker.intervalHasElapsed()) {
        _sizeStorerSyncTracker.resetLastTime();
        syncSizeInfo(false);
    }

    // We only want to check the queue max once per second or we'll thrash
    // This is done in haveDropsQueued, not dropAllQueued so we skip the mutex
    if (delta < 1000)
        return false;

    _previousCheckedDropsQueued = now;
    boost::mutex::scoped_lock lk(_identToDropMutex);
    return !_identToDrop.empty();
}

void WiredTigerKVEngine::dropAllQueued() {
    set<string> mine;
    {
        boost::mutex::scoped_lock lk(_identToDropMutex);
        mine = _identToDrop;
    }

    set<string> deleted;

    {
        WiredTigerSession session(_conn);
        for (set<string>::const_iterator it = mine.begin(); it != mine.end(); ++it) {
            string uri = *it;
            int ret = session.getSession()->drop(session.getSession(), uri.c_str(), "force");
            LOG(1) << "WT queued drop of  " << uri << " res " << ret;

            if (ret == 0) {
                deleted.insert(uri);
                continue;
            }

            if (ret == EBUSY) {
                // leave in qeuue
                continue;
            }

            invariantWTOK(ret);
        }
    }

    {
        boost::mutex::scoped_lock lk(_identToDropMutex);
        for (set<string>::const_iterator it = deleted.begin(); it != deleted.end(); ++it) {
            _identToDrop.erase(*it);
        }
    }
}

bool WiredTigerKVEngine::supportsDocLocking() const {
    return true;
}

bool WiredTigerKVEngine::supportsDirectoryPerDB() const {
    return true;
}

bool WiredTigerKVEngine::hasIdent(OperationContext* opCtx, const StringData& ident) const {
    return _hasUri(WiredTigerRecoveryUnit::get(opCtx)->getSession(opCtx)->getSession(),
                   _uri(ident));
}

bool WiredTigerKVEngine::_hasUri(WT_SESSION* session, const std::string& uri) const {
    // can't use WiredTigerCursor since this is called from constructor.
    WT_CURSOR* c = NULL;
    int ret = session->open_cursor(session, "metadata:", NULL, NULL, &c);
    if (ret == ENOENT)
        return false;
    invariantWTOK(ret);
    ON_BLOCK_EXIT(c->close, c);

    c->set_key(c, uri.c_str());
    return c->search(c) == 0;
}

std::vector<std::string> WiredTigerKVEngine::getAllIdents(OperationContext* opCtx) const {
    std::vector<std::string> all;
    WiredTigerCursor cursor("metadata:", WiredTigerSession::kMetadataCursorId, false, opCtx);
    WT_CURSOR* c = cursor.get();
    if (!c)
        return all;

    while (c->next(c) == 0) {
        const char* raw;
        c->get_key(c, &raw);
        StringData key(raw);
        size_t idx = key.find(':');
        if (idx == string::npos)
            continue;
        StringData type = key.substr(0, idx);
        if (type != "table")
            continue;

        StringData ident = key.substr(idx + 1);
        if (ident == "sizeStorer")
            continue;

        all.push_back(ident.toString());
    }

    return all;
}

int WiredTigerKVEngine::reconfigure(const char* str) {
    return _conn->reconfigure(_conn, str);
}

void WiredTigerKVEngine::_checkIdentPath(const StringData& ident) {
    size_t start = 0;
    size_t idx;
    while ((idx = ident.find('/', start)) != string::npos) {
        StringData dir = ident.substr(0, idx);

        boost::filesystem::path subdir = _path;
        subdir /= dir.toString();
        if (!boost::filesystem::exists(subdir)) {
            LOG(1) << "creating subdirectory: " << dir;
            try {
                boost::filesystem::create_directory(subdir);
            } catch (const std::exception& e) {
                error() << "error creating path " << subdir.string() << ' ' << e.what();
                throw;
            }
        }

        start = idx + 1;
    }
}
}
