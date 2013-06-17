// fts_search.cpp

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

#include "mongo/pch.h"

#include "mongo/db/btreecursor.h"
#include "mongo/db/fts/fts_index_format.h"
#include "mongo/db/fts/fts_search.h"
#include "mongo/db/kill_current_op.h"
#include "mongo/db/pdfile.h"

namespace mongo {

    namespace fts {

        /*
         * Constructor generates query and term dictionaries
         * @param ns, namespace
         * @param idxNum, index number
         * @param search, query string
         * @param language, language of the query
         * @param filter, filter object
         */
        FTSSearch::FTSSearch( NamespaceDetails* ns,
                              const IndexDetails& id,
                              const BSONObj& indexPrefix,
                              const FTSQuery& query,
                              const BSONObj& filter )
            : _ns( ns ),
              _id( id ),
              _fts( static_cast<FTSIndex*>(_id.getSpec().getType()) ),
              _indexPrefix( indexPrefix ),
              _query( query ),
              _ftsMatcher( query, static_cast<FTSIndex*>(_id.getSpec().getType())->getFtsSpec() ) {

            if ( !filter.isEmpty() )
                _matcher.reset( new CoveredIndexMatcher( filter, _fts->keyPattern() ) );

            _keysLookedAt = 0;
            _objectsLookedAt = 0;
        }

        bool FTSSearch::_ok( Record* record ) const {
            if ( !_query.hasNonTermPieces() )
                return true;
            return _ftsMatcher.matchesNonTerm( BSONObj::make( record ) );
        }

        /*
         * GO: sets the tree cursors on each term in terms,  processes the terms by advancing
         * the terms cursors and storing the partial
         * results and lastly calculates the top results
         * @param results, the priority queue containing the top results
         * @param limit, number of results in the priority queue
         */
        void FTSSearch::go(Results* results, unsigned limit ) {
            vector< shared_ptr<BtreeCursor> > cursors;

            for ( unsigned i = 0; i < _query.getTerms().size(); i++ ) {
                const string& term = _query.getTerms()[i];
                BSONObj min = FTSIndexFormat::getIndexKey( MAX_WEIGHT, term, _indexPrefix );
                BSONObj max = FTSIndexFormat::getIndexKey( 0, term, _indexPrefix );
                shared_ptr<BtreeCursor> c( BtreeCursor::make( _ns, _id, min, max, true, -1 ) );
                cursors.push_back( c );
            }

            while ( !inShutdown() ) {
                bool gotAny = false;
                for ( unsigned i = 0; i < cursors.size(); i++ ) {
                    if ( cursors[i]->eof() )
                        continue;
                    gotAny = true;
                    _process( cursors[i].get() );
                    cursors[i]->advance();
                }

                if ( !gotAny )
                    break;

                RARELY killCurrentOp.checkForInterrupt();
            }


            // priority queue using a compare that grabs the lowest of two ScoredLocations by score.
            for ( Scores::iterator i = _scores.begin(); i != _scores.end(); ++i ) {

                if ( i->second < 0 )
                    continue;

                // priority queue
                if ( results->size() < limit ) { // case a: queue unfilled

                    if ( !_ok( i->first ) )
                        continue;

                    results->push( ScoredLocation( i->first, i->second ) );

                }
                else if ( i->second > results->top().score ) { // case b: queue filled

                    if ( !_ok( i->first ) )
                        continue;

                    results->pop();
                    results->push( ScoredLocation( i->first, i->second ) );
                }
                else {
                    // else do nothing (case c)
                }

            }

        }

        /*
         * Takes a cursor and updates the partial score for said cursor in _scores map
         * @param cursor, btree cursor pointing to the current document to be scored
         */
        void FTSSearch::_process( BtreeCursor* cursor ) {
            _keysLookedAt++;

            BSONObj key = cursor->currKey();

            BSONObjIterator i( key );
            for ( unsigned j = 0; j < _fts->getFtsSpec().numExtraBefore(); j++ )
                i.next();
            i.next(); // move pase indexToken
            BSONElement scoreElement = i.next();

            double score = scoreElement.number();

            double& cur = _scores[(cursor->currLoc()).rec()];

            if ( cur < 0 ) {
                // already been rejected
                return;
            }

            if ( cur == 0 && _matcher.get() ) {
                // we haven't seen this before and we have a matcher
                MatchDetails d;
                if ( !_matcher->matchesCurrent( cursor, &d ) ) {
                    cur = -1;
                }

                if ( d.hasLoadedRecord() )
                    _objectsLookedAt++;

                if ( cur == -1 )
                    return;
            }

            if ( cur )
                cur += score * (1 + 1 / score);
            else
                cur += score;

        }

    }

}
