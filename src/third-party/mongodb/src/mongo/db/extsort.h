// extsort.h

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
*/

#pragma once

#include "mongo/pch.h"

#include "mongo/db/index.h"
#include "mongo/db/jsobj.h"
#include "mongo/db/namespace-inl.h"
#include "mongo/db/curop-inl.h"
#include "mongo/util/array.h"

namespace mongo {

    /**
       for external (disk) sorting by BSONObj and attaching a value
     */
    class BSONObjExternalSorter : boost::noncopyable {
    public:
        BSONObjExternalSorter( IndexInterface &i, const BSONObj & order = BSONObj() , long maxFileSize = 1024 * 1024 * 100 );
        ~BSONObjExternalSorter();
        typedef pair<BSONObj,DiskLoc> Data;
        /** @return the IndexInterface used to perform key comparisons. */
        const IndexInterface& getIndexInterface() const { return _idxi; }
 
    private:
        static HLMutex _extSortMutex;
        IndexInterface& _idxi;

        static int _compare(IndexInterface& i, const Data& l, const Data& r, const Ordering& order);

        class MyCmp {
        public:
            MyCmp( IndexInterface& i, BSONObj order = BSONObj() ) : _i(i), _order( Ordering::make(order) ) {}
            bool operator()( const Data &l, const Data &r ) const {
                return _compare(_i, l, r, _order) < 0;
            };
        private:
            IndexInterface& _i;
            const Ordering _order;
        };

        static IndexInterface *extSortIdxInterface;
        static Ordering extSortOrder;
        static bool extSortMayInterrupt;
        static int extSortComp( const void *lv, const void *rv );

        class FileIterator : boost::noncopyable {
        public:
            FileIterator( const std::string& file );
            ~FileIterator();
            bool more();
            Data next();
        private:
            bool _read( char* buf, long long count );

            int _file;
            unsigned long long _length;
            unsigned long long _readSoFar;
        };

    public:

        typedef FastArray<Data> InMemory;

        class Iterator : boost::noncopyable {
        public:

            Iterator( BSONObjExternalSorter * sorter );
            ~Iterator();
            bool more();
            Data next();

        private:
            MyCmp _cmp;
            vector<FileIterator*> _files;
            vector< pair<Data,bool> > _stash;

            InMemory * _in;
            InMemory::iterator _it;

        };

        void add( const BSONObj& o, const DiskLoc& loc, bool mayInterrupt );

        /* call after adding values, and before fetching the iterator */
        void sort( bool mayInterrupt );

        auto_ptr<Iterator> iterator() {
            uassert( 10052 ,  "not sorted" , _sorted );
            return auto_ptr<Iterator>( new Iterator( this ) );
        }

        int numFiles() {
            return _files.size();
        }

        long getCurSizeSoFar() { return _curSizeSoFar; }

        void hintNumObjects( long long numObjects ) {
            if ( numObjects < _arraySize )
                _arraySize = (int)(numObjects + 100);
        }

    private:

        void _sortInMem( bool mayInterrupt );

        void sort( const std::string& file );
        void finishMap( bool mayInterrupt );

        BSONObj _order;
        long _maxFilesize;
        boost::filesystem::path _root;

        int _arraySize;
        InMemory * _cur;
        long _curSizeSoFar;

        list<string> _files;
        bool _sorted;

        static unsigned long long _compares;
        static unsigned long long _uniqueNumber;
    };
}
