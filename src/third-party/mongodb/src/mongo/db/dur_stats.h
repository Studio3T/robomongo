// @file dur_stats.h

/**
*    Copyright (C) 2012 10gen Inc.
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
*/

namespace mongo {
    namespace dur {

        /** journaling stats.  the model here is that the commit thread is the only writer, and that reads are
            uncommon (from a serverStatus command and such).  Thus, there should not be multicore chatter overhead.
        */
        struct Stats {
            Stats();
            void rotate();
            BSONObj asObj();
            unsigned _intervalMicros;
            struct S {
                BSONObj _asObj();
                string _asCSV();
                string _CSVHeader();
                void reset();

                unsigned _commits;
                unsigned _earlyCommits; // count of early commits from commitIfNeeded() or from getDur().commitNow()
                unsigned long long _journaledBytes;
                unsigned long long _uncompressedBytes;
                unsigned long long _writeToDataFilesBytes;

                unsigned long long _prepLogBufferMicros;
                unsigned long long _writeToJournalMicros;
                unsigned long long _writeToDataFilesMicros;
                unsigned long long _remapPrivateViewMicros;

                // undesirable to be in write lock for the group commit (it can be done in a read lock), so good if we
                // have visibility when this happens.  can happen for a couple reasons
                // - read lock starvation
                // - file being closed
                // - data being written faster than the normal group commit interval
                unsigned _commitsInWriteLock;

                unsigned _dtMillis;
            };
            S *curr;
        private:
            S _a,_b;
            unsigned long long _lastRotate;
            S* other();
        };
        extern Stats stats;

    }
}
