// processinfo.h

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

#pragma once

#include <string>

#include "mongo/platform/cstdint.h"
#include "mongo/platform/process_id.h"
#include "mongo/db/jsobj.h"

namespace mongo {

class ProcessInfo {
public:
    ProcessInfo(ProcessId pid = ProcessId::getCurrent());
    ~ProcessInfo();

    /**
     * @return mbytes
     */
    int getVirtualMemorySize();

    /**
     * @return mbytes
     */
    int getResidentSize();

    /**
     * Get the type of os (e.g. Windows, Linux, Mac OS)
     */
    const std::string& getOsType() const {
        return sysInfo().osType;
    }

    /**
     * Get the os Name (e.g. Ubuntu, Gentoo, Windows Server 2008)
     */
    const std::string& getOsName() const {
        return sysInfo().osName;
    }

    /**
     * Get the os version (e.g. 10.04, 11.3.0, 6.1 (build 7600))
     */
    const std::string& getOsVersion() const {
        return sysInfo().osVersion;
    }

    /**
     * Get the cpu address size (e.g. 32, 36, 64)
     */
    unsigned getAddrSize() const {
        return sysInfo().addrSize;
    }

    /**
     * Get the total amount of system memory in MB
     */
    unsigned long long getMemSizeMB() const {
        return sysInfo().memSize / (1024 * 1024);
    }

    /**
     * Get the number of CPUs
     */
    unsigned getNumCores() const {
        return sysInfo().numCores;
    }

    /**
     * Get the system page size in bytes.
     */
    static unsigned long long getPageSize() {
        return systemInfo->pageSize;
    }

    /**
     * Get the CPU architecture (e.g. x86, x86_64)
     */
    const std::string& getArch() const {
        return sysInfo().cpuArch;
    }

    /**
     * Determine if NUMA is enabled (interleaved) for this process
     */
    bool hasNumaEnabled() const {
        return sysInfo().hasNuma;
    }

    /**
     * Determine if file zeroing is necessary for newly allocated data files.
     */
    static bool isDataFileZeroingNeeded() {
        return systemInfo->fileZeroNeeded;
    }

    /**
     * Determine if we need to workaround slow msync performance on Illumos/Solaris
     */
    static bool preferMsyncOverFSync() {
        return systemInfo->preferMsyncOverFSync;
    }

    /**
     * Get extra system stats
     */
    void appendSystemDetails(BSONObjBuilder& details) const {
        details.append(StringData("extra"), sysInfo()._extraStats.copy());
    }

    /**
     * Append platform-specific data to obj
     */
    void getExtraInfo(BSONObjBuilder& info);

    bool supported();

    static bool blockCheckSupported();

    static bool blockInMemory(const void* start);

    /**
     * Returns a positive floating point number between 0.0 and 1.0 to inform MMapV1 how much it
     * must remap pages to bring the system page file implementation back below a certain
     * threshold. A number of 1.0 means remap everything.
     */
    static double getSystemMemoryPressurePercentage();

    /**
     * @return a pointer aligned to the start of the page the provided pointer belongs to.
     *
     * NOTE requires blockCheckSupported() == true
     */
    inline static const void* alignToStartOfPage(const void* ptr) {
        return reinterpret_cast<const void*>(reinterpret_cast<unsigned long long>(ptr) &
                                             ~(getPageSize() - 1));
    }

    /**
     * Sets i-th element of 'out' to non-zero if the i-th page starting from the one containing
     * 'start' is in memory.
     * The 'out' vector will be resized to fit the requested number of pages.
     * @return true on success, false otherwise
     *
     * NOTE: requires blockCheckSupported() == true
     */
    static bool pagesInMemory(const void* start, size_t numPages, std::vector<char>* out);

private:
    /**
     * Host and operating system info.  Does not change over time.
     */
    class SystemInfo {
    public:
        std::string osType;
        std::string osName;
        std::string osVersion;
        unsigned addrSize;
        unsigned long long memSize;
        unsigned numCores;
        unsigned long long pageSize;
        std::string cpuArch;
        bool hasNuma;
        BSONObj _extraStats;

        // This is an OS specific value, which determines whether files should be zero-filled
        // at allocation time in order to avoid Microsoft KB 2731284.
        //
        bool fileZeroNeeded;

        // On non-Solaris (ie, Linux, Darwin, *BSD) kernels, prefer msync.
        // Illumos kernels do O(N) scans in memory of the page table during msync which
        // causes high CPU, Oracle Solaris 11.2 and later modified ZFS to workaround mongodb
        // Oracle Solaris Bug:
        //  18658199 Speed up msync() on ZFS by 90000x with this one weird trick
        bool preferMsyncOverFSync;

        SystemInfo()
            : addrSize(0),
              memSize(0),
              numCores(0),
              pageSize(0),
              hasNuma(false),
              fileZeroNeeded(false),
              preferMsyncOverFSync(true) {
            // populate SystemInfo during construction
            collectSystemInfo();
        }

    private:
        /** Collect host system info */
        void collectSystemInfo();
    };

    ProcessId _pid;
    static mongo::mutex _sysInfoLock;

    static bool checkNumaEnabled();

    static ProcessInfo::SystemInfo* systemInfo;

    inline const SystemInfo& sysInfo() const {
        return *systemInfo;
    }

public:
    static void initializeSystemInfo();
};

bool writePidFile(const std::string& path);

void printMemInfo(const char* whereContextStr = 0);
}
