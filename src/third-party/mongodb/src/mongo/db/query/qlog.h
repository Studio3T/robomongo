/**
 *    Copyright (C) 2013 10gen Inc.
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

#pragma once

#include "mongo/logger/log_component.h"
#include "mongo/logger/log_severity.h"
#include "mongo/logger/logger.h"
#include "mongo/logger/logstream_builder.h"
#include "mongo/util/concurrency/thread_name.h"

namespace mongo {

/**
 * Verbose query logging is determined by the 'Query' log level in the global log domain.
 * Set to verbosity 5 to enable.
 *
 * Command line:
 *     ./mongod --setParameter=logComponentVerbosity="{query: {verbosity: 5}}"
 *
 * Config file:
 *     systemLog:
 *         component:
 *             query:
 *                 verbosity: 5
 *
 * Shell:
 *     Enable:
 *         db.adminCommand({setParameter: 1, logComponentVerbosity: {query: {verbosity: 5}}})
 *     Disable:
 *         db.adminCommand({setParameter: 1, logComponentVerbosity: {query: {verbosity: -1}}})
 */
const logger::LogComponent verboseQueryLogComponent = logger::LogComponent::kQuery;
const logger::LogSeverity verboseQueryLogSeverity = logger::LogSeverity::Debug(5);

// With a #define like this, we don't evaluate the costly toString()s that are QLOG'd
#define QLOG()                                                                                   \
    if (!(::mongo::logger::globalLogDomain())                                                    \
             ->shouldLog(::mongo::verboseQueryLogComponent, ::mongo::verboseQueryLogSeverity)) { \
    } else                                                                                       \
    ::mongo::logger::LogstreamBuilder(::mongo::logger::globalLogDomain(),                        \
                                      ::mongo::getThreadName(),                                  \
                                      ::mongo::verboseQueryLogSeverity,                          \
                                      ::mongo::verboseQueryLogComponent)                         \
        << "[QLOG] "

bool qlogOff();
bool qlogOn();

}  // namespace mongo
