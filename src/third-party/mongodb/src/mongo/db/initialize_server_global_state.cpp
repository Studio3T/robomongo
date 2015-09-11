/**
*    Copyright (C) 2008 10gen Inc.
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

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include "mongo/db/initialize_server_global_state.h"

#include <boost/filesystem/operations.hpp>
#include <iostream>
#include <memory>
#include <signal.h>

#ifndef _WIN32
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include "mongo/base/init.h"
#include "mongo/client/sasl_client_authenticate.h"
#include "mongo/db/auth/authorization_manager.h"
#include "mongo/db/auth/authorization_manager_global.h"
#include "mongo/db/auth/internal_user_auth.h"
#include "mongo/db/auth/security_key.h"
#include "mongo/db/server_options.h"
#include "mongo/logger/logger.h"
#include "mongo/logger/console_appender.h"
#include "mongo/logger/message_event.h"
#include "mongo/logger/message_event_utf8_encoder.h"
#include "mongo/logger/ramlog.h"
#include "mongo/logger/rotatable_file_appender.h"
#include "mongo/logger/rotatable_file_manager.h"
#include "mongo/logger/rotatable_file_writer.h"
#include "mongo/logger/syslog_appender.h"
#include "mongo/platform/process_id.h"
#include "mongo/util/log.h"
#include "mongo/util/net/listen.h"
#include "mongo/util/net/ssl_manager.h"
#include "mongo/util/processinfo.h"
#include "mongo/util/quick_exit.h"

namespace fs = boost::filesystem;

namespace mongo {

using std::cerr;
using std::cout;
using std::endl;

#ifndef _WIN32
// support for exit value propagation with fork
void launchSignal(int sig) {
    if (sig == SIGUSR2) {
        ProcessId cur = ProcessId::getCurrent();

        if (cur == serverGlobalParams.parentProc || cur == serverGlobalParams.leaderProc) {
            // signal indicates successful start allowing us to exit
            quickExit(0);
        }
    }
}

static void setupLaunchSignals() {
    verify(signal(SIGUSR2, launchSignal) != SIG_ERR);
}

void signalForkSuccess() {
    if (serverGlobalParams.doFork) {
        // killing leader will propagate to parent
        verify(kill(serverGlobalParams.leaderProc.toNative(), SIGUSR2) == 0);
    }
}
#endif


static bool forkServer() {
#ifndef _WIN32
    if (serverGlobalParams.doFork) {
        fassert(16447, !serverGlobalParams.logpath.empty() || serverGlobalParams.logWithSyslog);

        cout.flush();
        cerr.flush();

        serverGlobalParams.parentProc = ProcessId::getCurrent();

        // facilitate clean exit when child starts successfully
        setupLaunchSignals();

        cout << "about to fork child process, waiting until server is ready for connections."
             << endl;

        pid_t child1 = fork();
        if (child1 == -1) {
            cout << "ERROR: stage 1 fork() failed: " << errnoWithDescription();
            quickExit(EXIT_ABRUPT);
        } else if (child1) {
            // this is run in the original parent process
            int pstat;
            waitpid(child1, &pstat, 0);

            if (WIFEXITED(pstat)) {
                if (WEXITSTATUS(pstat)) {
                    cout << "ERROR: child process failed, exited with error number "
                         << WEXITSTATUS(pstat) << endl;
                } else {
                    cout << "child process started successfully, parent exiting" << endl;
                }

                quickExit(WEXITSTATUS(pstat));
            }

            quickExit(50);
        }

        if (chdir("/") < 0) {
            cout << "Cant chdir() while forking server process: " << strerror(errno) << endl;
            quickExit(-1);
        }
        setsid();

        serverGlobalParams.leaderProc = ProcessId::getCurrent();

        pid_t child2 = fork();
        if (child2 == -1) {
            cout << "ERROR: stage 2 fork() failed: " << errnoWithDescription();
            quickExit(EXIT_ABRUPT);
        } else if (child2) {
            // this is run in the middle process
            int pstat;
            cout << "forked process: " << child2 << endl;
            waitpid(child2, &pstat, 0);

            if (WIFEXITED(pstat)) {
                quickExit(WEXITSTATUS(pstat));
            }

            quickExit(51);
        }

        // this is run in the final child process (the server)

        FILE* f = freopen("/dev/null", "w", stdout);
        if (f == NULL) {
            cout << "Cant reassign stdout while forking server process: " << strerror(errno)
                 << endl;
            return false;
        }

        f = freopen("/dev/null", "w", stderr);
        if (f == NULL) {
            cout << "Cant reassign stderr while forking server process: " << strerror(errno)
                 << endl;
            return false;
        }

        f = freopen("/dev/null", "r", stdin);
        if (f == NULL) {
            cout << "Cant reassign stdin while forking server process: " << strerror(errno) << endl;
            return false;
        }
    }
#endif  // !defined(_WIN32)
    return true;
}

void forkServerOrDie() {
    if (!forkServer())
        quickExit(EXIT_FAILURE);
}

MONGO_INITIALIZER_GENERAL(ServerLogRedirection,
                          ("GlobalLogManager", "EndStartupOptionHandling", "ForkServer"),
                          ("default"))(InitializerContext*) {
    using logger::LogManager;
    using logger::MessageEventEphemeral;
    using logger::MessageEventDetailsEncoder;
    using logger::MessageEventWithContextEncoder;
    using logger::MessageLogDomain;
    using logger::RotatableFileAppender;
    using logger::StatusWithRotatableFileWriter;

    if (serverGlobalParams.logWithSyslog) {
#ifdef _WIN32
        return Status(ErrorCodes::InternalError,
                      "Syslog requested in Windows build; command line processor logic error");
#else
        using logger::SyslogAppender;

        StringBuilder sb;
        sb << serverGlobalParams.binaryName << "." << serverGlobalParams.port;
        openlog(strdup(sb.str().c_str()), LOG_PID | LOG_CONS, serverGlobalParams.syslogFacility);
        LogManager* manager = logger::globalLogManager();
        manager->getGlobalDomain()->clearAppenders();
        manager->getGlobalDomain()->attachAppender(MessageLogDomain::AppenderAutoPtr(
            new SyslogAppender<MessageEventEphemeral>(new logger::MessageEventWithContextEncoder)));
        manager->getNamedDomain("javascriptOutput")
            ->attachAppender(
                MessageLogDomain::AppenderAutoPtr(new SyslogAppender<MessageEventEphemeral>(
                    new logger::MessageEventWithContextEncoder)));
#endif  // defined(_WIN32)
    } else if (!serverGlobalParams.logpath.empty()) {
        fassert(16448, !serverGlobalParams.logWithSyslog);
        std::string absoluteLogpath = boost::filesystem::absolute(serverGlobalParams.logpath,
                                                                  serverGlobalParams.cwd).string();

        bool exists;

        try {
            exists = boost::filesystem::exists(absoluteLogpath);
        } catch (boost::filesystem::filesystem_error& e) {
            return Status(ErrorCodes::FileNotOpen,
                          mongoutils::str::stream() << "Failed probe for \"" << absoluteLogpath
                                                    << "\": " << e.code().message());
        }

        if (exists) {
            if (boost::filesystem::is_directory(absoluteLogpath)) {
                return Status(ErrorCodes::FileNotOpen,
                              mongoutils::str::stream()
                                  << "logpath \"" << absoluteLogpath
                                  << "\" should name a file, not a directory.");
            }

            if (!serverGlobalParams.logAppend && boost::filesystem::is_regular(absoluteLogpath)) {
                std::string renameTarget = absoluteLogpath + "." + terseCurrentTime(false);
                if (0 == rename(absoluteLogpath.c_str(), renameTarget.c_str())) {
                    log() << "log file \"" << absoluteLogpath << "\" exists; moved to \""
                          << renameTarget << "\".";
                } else {
                    return Status(ErrorCodes::FileRenameFailed,
                                  mongoutils::str::stream()
                                      << "Could not rename preexisting log file \""
                                      << absoluteLogpath << "\" to \"" << renameTarget
                                      << "\"; run with --logappend or manually remove file: "
                                      << errnoWithDescription());
                }
            }
        }

        StatusWithRotatableFileWriter writer = logger::globalRotatableFileManager()->openFile(
            absoluteLogpath, serverGlobalParams.logAppend);
        if (!writer.isOK()) {
            return writer.getStatus();
        }

        LogManager* manager = logger::globalLogManager();
        manager->getGlobalDomain()->clearAppenders();
        manager->getGlobalDomain()->attachAppender(
            MessageLogDomain::AppenderAutoPtr(new RotatableFileAppender<MessageEventEphemeral>(
                new MessageEventDetailsEncoder, writer.getValue())));
        manager->getNamedDomain("javascriptOutput")
            ->attachAppender(
                MessageLogDomain::AppenderAutoPtr(new RotatableFileAppender<MessageEventEphemeral>(
                    new MessageEventDetailsEncoder, writer.getValue())));

        if (serverGlobalParams.logAppend && exists) {
            log() << "***** SERVER RESTARTED *****" << endl;
            Status status = logger::RotatableFileWriter::Use(writer.getValue()).status();
            if (!status.isOK())
                return status;
        }
    } else {
        logger::globalLogManager()
            ->getNamedDomain("javascriptOutput")
            ->attachAppender(MessageLogDomain::AppenderAutoPtr(
                new logger::ConsoleAppender<MessageEventEphemeral>(
                    new MessageEventDetailsEncoder)));
    }

    logger::globalLogDomain()->attachAppender(
        logger::MessageLogDomain::AppenderAutoPtr(new RamLogAppender(RamLog::get("global"))));

    return Status::OK();
}

/**
 * atexit handler to terminate the process before static destructors run.
 *
 * Mongo server processes cannot safely call ::exit() or std::exit(), but
 * some third-party libraries may call one of those functions.  In that
 * case, to avoid static-destructor problems in the server, this exits the
 * process immediately with code EXIT_FAILURE.
 *
 * TODO: Remove once exit() executes safely in mongo server processes.
 */
static void shortCircuitExit() {
    quickExit(EXIT_FAILURE);
}

MONGO_INITIALIZER(RegisterShortCircuitExitHandler)(InitializerContext*) {
    if (std::atexit(&shortCircuitExit) != 0)
        return Status(ErrorCodes::InternalError, "Failed setting short-circuit exit handler.");
    return Status::OK();
}

bool initializeServerGlobalState() {
    Listener::globalTicketHolder.resize(serverGlobalParams.maxConns);

#ifndef _WIN32
    if (!fs::is_directory(serverGlobalParams.socket)) {
        cout << serverGlobalParams.socket << " must be a directory" << endl;
        return false;
    }
#endif

    if (!serverGlobalParams.pidFile.empty()) {
        if (!writePidFile(serverGlobalParams.pidFile)) {
            // error message logged in writePidFile
            return false;
        }
    }

    int clusterAuthMode = serverGlobalParams.clusterAuthMode.load();
    if (!serverGlobalParams.keyFile.empty() &&
        clusterAuthMode != ServerGlobalParams::ClusterAuthMode_x509) {
        if (!setUpSecurityKey(serverGlobalParams.keyFile)) {
            // error message printed in setUpPrivateKey
            return false;
        }
    }

    // Auto-enable auth except if clusterAuthMode is not set.
    // clusterAuthMode is automatically set if a --keyFile parameter is provided.
    if (clusterAuthMode != ServerGlobalParams::ClusterAuthMode_undefined) {
        getGlobalAuthorizationManager()->setAuthEnabled(true);
    }

#ifdef MONGO_SSL

    if (clusterAuthMode == ServerGlobalParams::ClusterAuthMode_x509 ||
        clusterAuthMode == ServerGlobalParams::ClusterAuthMode_sendX509) {
        setInternalUserAuthParams(
            BSON(saslCommandMechanismFieldName
                 << "MONGODB-X509" << saslCommandUserDBFieldName << "$external"
                 << saslCommandUserFieldName
                 << getSSLManager()->getSSLConfiguration().clientSubjectName));
    }
#endif
    return true;
}

}  // namespace mongo
