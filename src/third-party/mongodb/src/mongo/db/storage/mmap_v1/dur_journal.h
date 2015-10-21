// @file dur_journal.h

/**
*    Copyright (C) 2010 10gen Inc.
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

namespace mongo {

class AlignedBuilder;
class JSectHeader;

namespace dur {

/** true if ok to cleanup journal files at termination. otherwise, files journal will be retained.
*/
extern bool okToCleanUp;

/** at termination after db files closed & fsynced
    also after recovery
    closes and removes journal files
    @param log report in log that we are cleaning up if we actually do any work
*/
void journalCleanup(bool log = false);

/** assure journal/ dir exists. throws */
void journalMakeDir();

/** check if time to rotate files; assure a file is open.
     done separately from the journal() call as we can do this part
     outside of lock.
    only called by durThread.
 */
void journalRotate();

/** flag that something has gone wrong during writing to the journal
    (not for recovery mode)
*/
void journalingFailure(const char* msg);

/** read lsn from disk from the last run before doing recovery */
unsigned long long journalReadLSN();

unsigned long long getLastDataFileFlushTime();

/** never throws.
    @param anyFiles by default we only look at j._* files. If anyFiles is true, return true
           if there are any files in the journal directory. acquirePathLock() uses this to
           make sure that the journal directory is mounted.
    @return true if there are any journal files in the journal dir.
*/
bool haveJournalFiles(bool anyFiles = false);

/**
 * Writes the specified uncompressed buffer to the journal.
 */
void WRITETOJOURNAL(const JSectHeader& h, const AlignedBuilder& uncompressed);

// in case disk controller buffers writes
const long long ExtraKeepTimeMs = 10000;

const unsigned JournalCommitIntervalDefault = 100;
}
}
