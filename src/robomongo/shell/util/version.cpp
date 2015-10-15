// @file version.cpp

/*    Copyright 2009 10gen Inc.
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
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kControl

#include "mongo/platform/basic.h"

#include <initializer_list>
#include <boost/version.hpp>
#include <sstream>
#include <string>

#include "mongo/base/parse_number.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/db/jsobj.h"
#include "mongo/util/debug_util.h"
#include "mongo/util/log.h"
#include "mongo/util/mongoutils/str.h"
#include "mongo/util/version.h"

#ifdef MONGO_SSL
#include <openssl/crypto.h>
#endif

#include <pcrecpp.h>
#include <cstdlib>

namespace mongo {

    using std::string;
    using std::stringstream;

    const char versionString[] = "@mongo_version@";
    const char * gitVersion() { return "@mongo_git_version@"; }
    const char * compiledJSEngine() { return "@buildinfo_js_engine@"; }
    const char * allocator() { return "@buildinfo_allocator@"; }
    const char * compilerFlags() { return "@buildinfo_compiler_flags@"; }
    const char * loaderFlags() { return "@buildinfo_loader_flags@"; }
    const char * sysInfo() { return "@buildinfo_sysinfo@ BOOST_LIB_VERSION=" BOOST_LIB_VERSION ; }

    const int kMongoVersionMajor = @mongo_version_major@;
    const int kMongoVersionMinor = @mongo_version_minor@;
    const int kMongoVersionPatch = @mongo_version_patch@;
    const int kMongoVersionExtra = @mongo_version_extra@;
    const char kMongoVersionExtraStr[] = "@mongo_version_extra_str@";

    bool isSameMajorVersion(const char* version) {
        int major = -1, minor = -1;
        pcrecpp::RE ver_regex("^(\\d+)\\.(\\d+)\\.");
        ver_regex.PartialMatch(version, &major, &minor);

        if(major == -1 || minor == -1)
            return false;
        return (major == kMongoVersionMajor && minor == kMongoVersionMinor);
    }

    string mongodVersion() {
        stringstream ss;
        ss << "db version v" << versionString;
        return ss.str();
    }

    void printGitVersion() { log() << "git version: " << gitVersion(); }

    const std::string openSSLVersion(const std::string &prefix, const std::string &suffix) {
#ifndef MONGO_SSL
        return "";
#else
        return prefix + SSLeay_version(SSLEAY_VERSION) + suffix;
#endif
    }

    void printOpenSSLVersion() {
#ifdef MONGO_CONFIG_SSL
        log() << openSSLVersion("OpenSSL version: ");
#endif
    }

#if defined(_WIN32)
    const char * targetMinOS() {
#if (NTDDI_VERSION >= NTDDI_WIN7)
        return "Windows 7/Windows Server 2008 R2";
#elif (NTDDI_VERSION >= NTDDI_VISTA)
        return "Windows Vista/Windows Server 2008";
#elif (NTDDI_VERSION >= NTDDI_WS03SP2)
        return "Windows Server 2003 SP2";
#elif (NTDDI_VERSION >= NTDDI_WINXPSP3)
        return "Windows XP SP3";
#else
#error This targeted Windows version is not supported
#endif // NTDDI_VERSION
    }

    void printTargetMinOS() {
        log() << "targetMinOS: " << targetMinOS();
    }
#endif // _WIN32

    void printAllocator() {
        log() << "allocator: " << allocator() << std::endl;
    }

    void printSysInfo() {
        log() << "build info: " << sysInfo() << std::endl;
    }

    void appendBuildInfo(BSONObjBuilder& result) {
       result << "version" << versionString
              << "gitVersion" << gitVersion()
#if defined(_WIN32)
              << "targetMinOS" << targetMinOS()
#endif
              << "OpenSSLVersion" << openSSLVersion()
              << "sysInfo" << sysInfo();

       BSONArrayBuilder versionArray(result.subarrayStart("versionArray"));
       versionArray << kMongoVersionMajor
                    << kMongoVersionMinor
                    << kMongoVersionPatch
                    << kMongoVersionExtra;
       versionArray.done();

       result << "loaderFlags" << loaderFlags()
              << "compilerFlags" << compilerFlags()
              << "allocator" << allocator()
              << "javascriptEngine" << compiledJSEngine()
              << "bits" << ( sizeof( int* ) == 4 ? 32 : 64 );
       result.appendBool( "debug" , debug );
       result.appendNumber("maxBsonObjectSize", BSONObjMaxUserSize);
    }
}
