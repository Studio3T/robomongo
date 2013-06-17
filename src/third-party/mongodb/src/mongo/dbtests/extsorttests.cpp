//@file extsorttests.cpp : mongo/db/extsort.{h,cpp} tests

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

#include "mongo/db/extsort.h"

#include "mongo/db/pdfile.h"
#include "mongo/platform/cstdint.h"

#include "mongo/dbtests/dbtests.h"

namespace ExtSortTests {

    bool isSolaris() {
#ifdef __sunos__
        return true;
#else
        return false;
#endif
    }

    static const char* const _ns = "unittests.extsort";
    DBDirectClient _client;
    IndexInterface& _arbitraryIndexInterface = *IndexDetails::iis[ time( 0 ) % 2 ];

    /** Sort four values. */
    class SortFour {
    public:
        void run() {
            BSONObjExternalSorter sorter( _arbitraryIndexInterface );

            sorter.add( BSON( "x" << 10 ), DiskLoc( 5, 1 ), false );
            sorter.add( BSON( "x" << 2 ), DiskLoc( 3, 1 ), false );
            sorter.add( BSON( "x" << 5 ), DiskLoc( 6, 1 ), false );
            sorter.add( BSON( "x" << 5 ), DiskLoc( 7, 1 ), false );

            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            int num=0;
            while ( i->more() ) {
                pair<BSONObj,DiskLoc> p = i->next();
                if ( num == 0 )
                    ASSERT_EQUALS( 2, p.first["x"].number() );
                else if ( num <= 2 ) {
                    ASSERT_EQUALS( 5, p.first["x"].number() );
                }
                else if ( num == 3 )
                    ASSERT_EQUALS( 10, p.first["x"].number() );
                else
                    ASSERT( false );
                num++;
            }

            ASSERT_EQUALS( 0 , sorter.numFiles() );
        }
    };

    /** Sort four values and check disk locs. */
    class SortFourCheckDiskLoc {
    public:
        void run() {
            BSONObjExternalSorter sorter( _arbitraryIndexInterface, BSONObj(), 10 );
            sorter.add( BSON( "x" << 10 ), DiskLoc( 5, 11 ), false );
            sorter.add( BSON( "x" << 2 ), DiskLoc( 3, 1 ), false );
            sorter.add( BSON( "x" << 5 ), DiskLoc( 6, 1 ), false );
            sorter.add( BSON( "x" << 5 ), DiskLoc( 7, 1 ), false );

            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            int num=0;
            while ( i->more() ) {
                pair<BSONObj,DiskLoc> p = i->next();
                if ( num == 0 ) {
                    ASSERT_EQUALS( 2, p.first["x"].number() );
                    ASSERT_EQUALS( "3:1", p.second.toString() );
                }
                else if ( num <= 2 )
                    ASSERT_EQUALS( 5, p.first["x"].number() );
                else if ( num == 3 ) {
                    ASSERT_EQUALS( 10, p.first["x"].number() );
                    ASSERT_EQUALS( "5:b", p.second.toString() );
                }
                else
                    ASSERT( false );
                num++;
            }
        }
    };

    /** Sort no values. */
    class SortNone {
    public:
        void run() {
            BSONObjExternalSorter sorter( _arbitraryIndexInterface, BSONObj(), 10 );
            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            ASSERT( ! i->more() );
        }
    };

    /** Check sorting by disk location. */
    class SortByDiskLock {
    public:
        void run() {
            BSONObjExternalSorter sorter( _arbitraryIndexInterface );
            sorter.add( BSON( "x" << 10 ), DiskLoc( 5, 4 ), false );
            sorter.add( BSON( "x" << 2 ), DiskLoc( 3, 0 ), false );
            sorter.add( BSON( "x" << 5 ), DiskLoc( 6, 2 ), false );
            sorter.add( BSON( "x" << 5 ), DiskLoc( 7, 3 ), false );
            sorter.add( BSON( "x" << 5 ), DiskLoc( 2, 1 ), false );

            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            int num=0;
            while ( i->more() ) {
                pair<BSONObj,DiskLoc> p = i->next();
                if ( num == 0 )
                    ASSERT_EQUALS( 2, p.first["x"].number() );
                else if ( num <= 3 ) {
                    ASSERT_EQUALS( 5, p.first["x"].number() );
                }
                else if ( num == 4 )
                    ASSERT_EQUALS( 10, p.first["x"].number() );
                else
                    ASSERT( false );
                ASSERT_EQUALS( num , p.second.getOfs() );
                num++;
            }
        }
    };

    /** Sort 1e4 values. */
    class Sort1e4 {
    public:
        void run() {
            BSONObjExternalSorter sorter( _arbitraryIndexInterface, BSONObj() , 2000 );
            for ( int i=0; i<10000; i++ ) {
                sorter.add( BSON( "x" << rand() % 10000 ), DiskLoc( 5, i ), false );
            }

            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            int num=0;
            double prev = 0;
            while ( i->more() ) {
                pair<BSONObj,DiskLoc> p = i->next();
                num++;
                double cur = p.first["x"].number();
                ASSERT( cur >= prev );
                prev = cur;
            }
            ASSERT_EQUALS( 10000, num );
        }
    };

    /** Sort 1e5 values. */
    class Sort1e5 {
    public:
        void run() {
            const int total = 100000;
            BSONObjExternalSorter sorter( _arbitraryIndexInterface, BSONObj() , total * 2 );
            for ( int i=0; i<total; i++ ) {
                sorter.add( BSON( "a" << "b" ), DiskLoc( 5, i ), false );
            }

            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            int num=0;
            double prev = 0;
            while ( i->more() ) {
                pair<BSONObj,DiskLoc> p = i->next();
                num++;
                double cur = p.first["x"].number();
                ASSERT( cur >= prev );
                prev = cur;
            }
            ASSERT_EQUALS( total, num );
            ASSERT( sorter.numFiles() > 2 );
        }
    };

    /** Sort 1e6 values. */
    class Sort1e6 {
    public:
        void run() {
            const int total = 1000 * 1000;
            BSONObjExternalSorter sorter( _arbitraryIndexInterface, BSONObj() , total * 2 );
            for ( int i=0; i<total; i++ ) {
                sorter.add( BSON( "abcabcabcabd" << "basdasdasdasdasdasdadasdasd" << "x" << i ),
                            DiskLoc( 5, i ),
                            false );
            }

            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            int num=0;
            double prev = 0;
            while ( i->more() ) {
                pair<BSONObj,DiskLoc> p = i->next();
                num++;
                double cur = p.first["x"].number();
                ASSERT( cur >= prev );
                prev = cur;
            }
            ASSERT_EQUALS( total, num );
            ASSERT( sorter.numFiles() > 2 );
        }
    };

    /** Sort null valued keys. */
    class SortNull {
    public:
        void run() {

            BSONObjBuilder b;
            b.appendNull("");
            BSONObj x = b.obj();

            BSONObjExternalSorter sorter( _arbitraryIndexInterface );
            sorter.add(x, DiskLoc(3,7), false);
            sorter.add(x, DiskLoc(4,7), false);
            sorter.add(x, DiskLoc(2,7), false);
            sorter.add(x, DiskLoc(1,7), false);
            sorter.add(x, DiskLoc(3,77), false);

            sorter.sort( false );

            auto_ptr<BSONObjExternalSorter::Iterator> i = sorter.iterator();
            while( i->more() ) {
                BSONObjExternalSorter::Data d = i->next();
            }
        }
    };

    /** Sort 130 keys and check their exact values. */
    class Sort130 {
    public:
        void run() {
            // Create a sorter.
            BSONObjExternalSorter sorter( IndexInterface::defaultVersion(), BSON( "a" << 1 ) );
            // Add keys to the sorter.
            int32_t nDocs = 130;
            for( int32_t i = 0; i < nDocs; ++i ) {
                // Insert values in reverse order, for subsequent sort.
                sorter.add( BSON( "" << ( nDocs - 1 - i ) ), /* dummy disk loc */ DiskLoc(), true );
            }
            // The sorter's footprint is now positive.
            ASSERT( sorter.getCurSizeSoFar() > 0 );
            // Sort the keys.
            sorter.sort( true );
            // Check that the keys have been sorted.
            auto_ptr<BSONObjExternalSorter::Iterator> iterator = sorter.iterator();
            int32_t expectedKey = 0;
            while( iterator->more() ) {
                ASSERT_EQUALS( BSON( "" << expectedKey++ ), iterator->next().first );
            }
            ASSERT_EQUALS( nDocs, expectedKey );
        }
    };

    /**
     * BSONObjExternalSorter::add() aborts if the current operation is interrupted, even if storage
     * system writes have occurred.
     */
    class InterruptAdd {
    public:
        InterruptAdd( bool mayInterrupt ) :
            _mayInterrupt( mayInterrupt ) {
        }
        void run() {
            _client.createCollection( _ns );
            // Take a write lock.
            Client::WriteContext ctx( _ns );
            // Do a write to ensure the implementation will interrupt sort() even after a write has
            // occurred.
            BSONObj newDoc;
            theDataFileMgr.insertWithObjMod( _ns, newDoc );
            // Create a sorter with a max file size of only 10k, to trigger a file flush after a
            // relatively small number of inserts.
            BSONObjExternalSorter sorter( IndexInterface::defaultVersion(),
                                          BSON( "a" << 1 ),
                                          10 * 1024 );
            // Register a request to kill the current operation.
            cc().curop()->kill();
            if ( _mayInterrupt && !isSolaris() ) { // This interrupt is unsupported on solaris.
                // When enough keys are added to fill the first file, an interruption will be
                // triggered as the records are sorted for the file.
                ASSERT_THROWS( addKeysUntilFileFlushed( &sorter, _mayInterrupt ), UserException );
            }
            else {
                // When enough keys are added to fill the first file, an interruption when the
                // records are sorted for the file is prevented because mayInterrupt == false.
                addKeysUntilFileFlushed( &sorter, _mayInterrupt );
            }
        }
    private:
        static void addKeysUntilFileFlushed( BSONObjExternalSorter* sorter, bool mayInterrupt ) {
            while( sorter->numFiles() == 0 ) {
                sorter->add( BSON( "" << 1 ), /* dummy disk loc */ DiskLoc(), mayInterrupt );
            }
        }
        bool _mayInterrupt;
    };

    /**
     * BSONObjExternalSorter::sort() aborts if the current operation is interrupted, even if storage
     * system writes have occurred.
     */
    class InterruptSort {
    public:
        InterruptSort( bool mayInterrupt ) :
            _mayInterrupt( mayInterrupt ) {
        }
        void run() {
            _client.createCollection( _ns );
            // Take a write lock.
            Client::WriteContext ctx( _ns );
            // Do a write to ensure the implementation will interrupt sort() even after a write has
            // occurred.
            BSONObj newDoc;
            theDataFileMgr.insertWithObjMod( _ns, newDoc );
            // Create a sorter.
            BSONObjExternalSorter sorter( IndexInterface::defaultVersion(), BSON( "a" << 1 ) );
            // Add keys to the sorter.
            int32_t nDocs = 130;
            for( int32_t i = 0; i < nDocs; ++i ) {
                sorter.add( BSON( "" << i ), /* dummy disk loc */ DiskLoc(), false );
            }
            ASSERT( sorter.getCurSizeSoFar() > 0 );
            // Register a request to kill the current operation.
            cc().curop()->kill();
            if ( _mayInterrupt && !isSolaris() ) { // This interrupt is unsupported on solaris.
                // The sort is aborted due to the kill request.
                ASSERT_THROWS( sorter.sort( _mayInterrupt ), UserException );
                // TODO Check that an iterator cannot be retrieved because the keys are unsorted (Not
                // currently implemented.)
                if ( 0 ) {
                    ASSERT_THROWS( sorter.iterator(), UserException );
                }
            }
            else {
                // Sort the keys.
                sorter.sort( _mayInterrupt );
                // Check that the keys have been sorted.
                auto_ptr<BSONObjExternalSorter::Iterator> iterator = sorter.iterator();
                int32_t expectedKey = 0;
                while( iterator->more() ) {
                    ASSERT_EQUALS( BSON( "" << expectedKey++ ), iterator->next().first );
                }
                ASSERT_EQUALS( nDocs, expectedKey );
            }
        }
    private:
        bool _mayInterrupt;
    };

    class ExtSortTests : public Suite {
    public:
        ExtSortTests() :
            Suite( "extsort" ) {
        }

        void setupTests() {
            add<SortFour>();
            add<SortFourCheckDiskLoc>();
            add<SortNone>();
            add<SortByDiskLock>();
            add<Sort1e4>();
            add<Sort1e5>();
            add<Sort1e6>();
            add<SortNull>();
            add<Sort130>();
            add<InterruptAdd>( false );
            add<InterruptAdd>( true );
            add<InterruptSort>( false );
            add<InterruptSort>( true );
        }
    } extSortTests;

} // namespace ExtSortTests
