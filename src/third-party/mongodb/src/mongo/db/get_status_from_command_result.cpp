/**
 *    Copyright (C) 2014 MongoDB Inc.
 *
 *    This program is free software: you can redistribute it and/or  modify
 *    it under the terms of the GNU Affero General Public License, version 3,
 *    as published by the Free Software Foundation.
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

#include "mongo/platform/basic.h"

#include "mongo/db/get_status_from_command_result.h"

#include "mongo/base/status.h"
#include "mongo/db/jsobj.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {

Status getStatusFromCommandResult(const BSONObj& result) {
    BSONElement okElement = result["ok"];
    BSONElement codeElement = result["code"];
    BSONElement errmsgElement = result["errmsg"];
    if (okElement.eoo()) {
        return Status(ErrorCodes::CommandResultSchemaViolation,
                      mongoutils::str::stream() << "No \"ok\" field in command result " << result);
    }
    if (okElement.trueValue()) {
        return Status::OK();
    }
    int code = codeElement.numberInt();
    if (0 == code) {
        code = ErrorCodes::UnknownError;
    }
    std::string errmsg;
    if (errmsgElement.type() == String) {
        errmsg = errmsgElement.String();
    } else if (!errmsgElement.eoo()) {
        errmsg = errmsgElement.toString();
    }
    return Status(ErrorCodes::Error(code), errmsg);
}

}  // namespace mongo
