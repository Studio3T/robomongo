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

#pragma once

#include "../goodies.h" // printStackTrace
#include "mutexdebugger.h"
#include "mongo/util/stacktrace.h"

namespace mongo {

    namespace race {

#ifdef _WIN32
    typedef unsigned threadId_t;
#else
    typedef pthread_t threadId_t;
#endif

#if defined(_DEBUG)

        class Block { 
            volatile int n;
            unsigned ncalls;
            const string file;
            const unsigned line;
            void fail() { 
                log() << "\n\n\nrace: synchronization (race condition) failure\ncurrent locks this thread (" << getThreadName() << "):" << endl
                    << mutexDebugger.currentlyLocked() << endl;
                printStackTrace();
                ::abort();
            }
            void enter() { 
                if( ++n != 1 ) fail();
                ncalls++;
                if( ncalls < 100 ) {
                    sleepmillis(0);
                }
                else {
                    RARELY {
                        sleepmillis(0);
                        if( ncalls < 128 * 20 ) {
                            OCCASIONALLY { 
                                sleepmillis(3);
                            }
                        }
                    }
                }
            }
            void leave() {
                if( --n != 0 ) fail();
            }
        public:
            Block(const std::string& f, unsigned l) : n(0), ncalls(0), file(f), line(l) { }
            ~Block() { 
                if( ncalls > 1000000 ) { 
                    // just so we know if we are slowing things down
                    log() << "race::Block lots of calls " << file << ' ' << line << " n:" << ncalls << endl;
                }
            }
            class Within { 
                Block& _s;
            public:
                Within(Block& s) : _s(s) { _s.enter(); }
                ~Within() { _s.leave(); }
            };
        };
 
        /* in a rwlock situation this will fail, so not appropriate for things like that. */
# define RACECHECK \
        static race::Block __cp(__FILE__, __LINE__); \
        race::Block::Within __ck(__cp);

#else
        /* !_DEBUG */
# define RACECHECK

#endif

    }
}
