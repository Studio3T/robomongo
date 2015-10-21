/*    Copyright 2012 10gen Inc.
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

#include <cstdlib>
#include <string>

#include <kvm.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/vmmeter.h>
#include <unistd.h>
#include <vm/vm_param.h>

#include "mongo/util/scopeguard.h"
#include "mongo/util/log.h"
#include "processinfo.h"

using namespace std;

namespace mongo {

ProcessInfo::ProcessInfo(ProcessId pid) : _pid(pid) {}

ProcessInfo::~ProcessInfo() {}

/**
 * Get a sysctl string value by name.  Use string specialization by default.
 */
template <typename T>
int getSysctlByNameWithDefault(const char* sysctlName, const T& defaultValue, T* result);

template <>
int getSysctlByNameWithDefault<uintptr_t>(const char* sysctlName,
                                          const uintptr_t& defaultValue,
                                          uintptr_t* result) {
    uintptr_t value = 0;
    size_t len = sizeof(value);
    if (sysctlbyname(sysctlName, &value, &len, NULL, 0) == -1) {
        *result = defaultValue;
        return errno;
    }
    if (len > sizeof(value)) {
        *result = defaultValue;
        return EINVAL;
    }

    *result = value;
    return 0;
}

template <>
int getSysctlByNameWithDefault<string>(const char* sysctlName,
                                       const string& defaultValue,
                                       string* result) {
    char value[256] = {0};
    size_t len = sizeof(value);
    if (sysctlbyname(sysctlName, &value, &len, NULL, 0) == -1) {
        *result = defaultValue;
        return errno;
    }
    *result = value;
    return 0;
}

bool ProcessInfo::checkNumaEnabled() {
    return false;
}

int ProcessInfo::getVirtualMemorySize() {
    kvm_t* kd = NULL;
    int cnt = 0;
    char err[_POSIX2_LINE_MAX] = {0};
    if ((kd = kvm_open(NULL, "/dev/null", "/dev/null", O_RDONLY, err)) == NULL)
        return -1;
    kinfo_proc* task = kvm_getprocs(kd, KERN_PROC_PID, _pid.toNative(), &cnt);
    kvm_close(kd);
    return task->ki_size / 1024 / 1024;  // convert from bytes to MB
}

int ProcessInfo::getResidentSize() {
    kvm_t* kd = NULL;
    int cnt = 0;
    char err[_POSIX2_LINE_MAX] = {0};
    if ((kd = kvm_open(NULL, "/dev/null", "/dev/null", O_RDONLY, err)) == NULL)
        return -1;
    kinfo_proc* task = kvm_getprocs(kd, KERN_PROC_PID, _pid.toNative(), &cnt);
    kvm_close(kd);
    return task->ki_rssize * sysconf(_SC_PAGESIZE) / 1024 / 1024;  // convert from pages to MB
}

double ProcessInfo::getSystemMemoryPressurePercentage() {
    return 0.0;
}

void ProcessInfo::SystemInfo::collectSystemInfo() {
    osType = "BSD";
    osName = "FreeBSD";

    int status = getSysctlByNameWithDefault("kern.version", string("unknown"), &osVersion);
    if (status != 0)
        log() << "Unable to collect OS Version. (errno: " << status << " msg: " << strerror(status)
              << ")" << endl;

    status = getSysctlByNameWithDefault("hw.machine_arch", string("unknown"), &cpuArch);
    if (status != 0)
        log() << "Unable to collect Machine Architecture. (errno: " << status
              << " msg: " << strerror(status) << ")" << endl;
    addrSize = cpuArch.find("64") != std::string::npos ? 64 : 32;

    uintptr_t numBuffer;
    uintptr_t defaultNum = 1;
    status = getSysctlByNameWithDefault("hw.physmem", defaultNum, &numBuffer);
    memSize = numBuffer;
    if (status != 0)
        log() << "Unable to collect Physical Memory. (errno: " << status
              << " msg: " << strerror(status) << ")" << endl;

    status = getSysctlByNameWithDefault("hw.ncpu", defaultNum, &numBuffer);
    numCores = numBuffer;
    if (status != 0)
        log() << "Unable to collect Number of CPUs. (errno: " << status
              << " msg: " << strerror(status) << ")" << endl;

    pageSize = static_cast<unsigned long long>(sysconf(_SC_PAGESIZE));

    hasNuma = checkNumaEnabled();
}

void ProcessInfo::getExtraInfo(BSONObjBuilder& info) {}

bool ProcessInfo::supported() {
    return true;
}

bool ProcessInfo::blockCheckSupported() {
    return true;
}

bool ProcessInfo::blockInMemory(const void* start) {
    char x = 0;
    if (mincore(alignToStartOfPage(start), getPageSize(), &x)) {
        log() << "mincore failed: " << errnoWithDescription() << endl;
        return 1;
    }
    return x & 0x1;
}

bool ProcessInfo::pagesInMemory(const void* start, size_t numPages, vector<char>* out) {
    out->resize(numPages);
    // int mincore(const void *addr, size_t len, char *vec);
    if (mincore(alignToStartOfPage(start), numPages * getPageSize(), &(out->front()))) {
        log() << "mincore failed: " << errnoWithDescription() << endl;
        return false;
    }
    for (size_t i = 0; i < numPages; ++i) {
        (*out)[i] = 0x1;
    }
    return true;
}
}
