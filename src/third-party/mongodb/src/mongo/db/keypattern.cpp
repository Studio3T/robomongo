// @file keypattern.cpp

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

#include "mongo/db/keypattern.h"

#include "mongo/db/hasher.h"
#include "mongo/db/queryutil.h"
#include "mongo/util/mongoutils/str.h"

using namespace mongoutils;

namespace mongo {

    KeyPattern::KeyPattern( const BSONObj& pattern ): _pattern( pattern ) {

        // Extract all prefixes of each field in pattern.
        BSONForEach( field, _pattern ) {
            StringData fieldName = field.fieldName();
            size_t pos = fieldName.find( '.' );
            while ( pos != string::npos ) {
                _prefixes.insert( StringData( field.fieldName(), pos ) );
                pos = fieldName.find( '.', pos+1 );
            }
            _prefixes.insert( fieldName );
        }
    }

    BSONObj KeyPattern::extractSingleKey(const BSONObj& doc ) const {
        if ( _pattern.isEmpty() )
            return BSONObj();

        if ( mongoutils::str::equals( _pattern.firstElement().valuestrsafe() , "hashed" ) ){
            BSONElement fieldVal = doc.getFieldDotted( _pattern.firstElementFieldName() );
            return BSON( _pattern.firstElementFieldName() <<
                         BSONElementHasher::hash64( fieldVal ,
                                                    BSONElementHasher::DEFAULT_HASH_SEED ) );
        }

        return doc.extractFields( _pattern );
    }

    bool KeyPattern::isSpecial() const {
        BSONForEach(e, _pattern) {
            int fieldVal = e.numberInt();
            if ( fieldVal != 1 && fieldVal != -1 ){
                return true;
            }
        }
        return false;
    }

    bool KeyPattern::isCoveredBy( const KeyPattern& other ) const {
        BSONForEach( e, _pattern ) {
            BSONElement otherfield = other.getField( e.fieldName() );
            if ( otherfield.eoo() ){
                return false;
            }

            if ( otherfield.numberInt() != 1 && otherfield.numberInt() != -1 && otherfield != e ){
                return false;
            }
        }
        return true;
    }

    BSONObj KeyPattern::extendRangeBound( const BSONObj& bound , bool makeUpperInclusive ) const {
        BSONObjBuilder newBound( bound.objsize() );

        BSONObjIterator src( bound );
        BSONObjIterator pat( _pattern );

        while( src.more() ){
            massert( 16649 ,
                     str::stream() << "keyPattern " << _pattern << " shorter than bound " << bound,
                     pat.more() );
            BSONElement srcElt = src.next();
            BSONElement patElt = pat.next();
            massert( 16634 ,
                     str::stream() << "field names of bound " << bound
                                   << " do not match those of keyPattern " << _pattern ,
                                   str::equals( srcElt.fieldName() , patElt.fieldName() ) );
            newBound.append( srcElt );
        }
        while( pat.more() ){
            BSONElement patElt = pat.next();
            // for non 1/-1 field values, like {a : "hashed"}, treat order as ascending
            int order = patElt.isNumber() ? patElt.numberInt() : 1;
            // flip the order semantics if this is an upper bound
            if ( makeUpperInclusive ) order *= -1;

            if( order > 0 ){
                newBound.appendMinKey( patElt.fieldName() );
            }
            else {
                newBound.appendMaxKey( patElt.fieldName() );
            }
        }
        return newBound.obj();
    }

    typedef vector<pair<BSONObj,BSONObj> >::const_iterator BoundListIter;

    BoundList KeyPattern::keyBounds( const FieldRangeSet& queryConstraints ) const {
        // To construct our bounds we will generate intervals based on constraints for
        // the first field, then compound intervals based on constraints for the first
        // 2 fields, then compound intervals for the first 3 fields, etc.
        // As we loop through the fields, we start generating new intervals that will later
        // get extended in another iteration of the loop.  We define these partially constructed
        // intervals using pairs of BSONObjBuilders (shared_ptrs, since after one iteration of the
        // loop they still must exist outside their scope).
        typedef vector< pair< shared_ptr<BSONObjBuilder> ,
                              shared_ptr<BSONObjBuilder> > > BoundBuilders;
        BoundBuilders builders;
        builders.push_back( make_pair( shared_ptr<BSONObjBuilder>( new BSONObjBuilder() ),
                                       shared_ptr<BSONObjBuilder>( new BSONObjBuilder() ) ) );
        BSONObjIterator i( _pattern );
        // until equalityOnly is false, we are just dealing with equality (no range or $in queries).
        bool equalityOnly = true;
        while( i.more() ) {
            BSONElement e = i.next();

            // get the relevant intervals for this field, but we may have to transform the
            // list of what's relevant according to the expression for this field
            const FieldRange &fr = queryConstraints.range( e.fieldName() );
            const vector<FieldInterval> &oldIntervals = fr.intervals();
            BoundList fieldBounds = _transformFieldBounds( oldIntervals , e );

            if ( equalityOnly ) {
                if ( fieldBounds.size() == 1 &&
                     ( fieldBounds.front().first == fieldBounds.front().second ) ){
                    // this field is only a single point-interval
                    BoundBuilders::const_iterator j;
                    for( j = builders.begin(); j != builders.end(); ++j ) {
                        j->first->appendElements( fieldBounds.front().first );
                        j->second->appendElements( fieldBounds.front().first );
                    }
                }
                else {
                    // This clause is the first to generate more than a single point.
                    // We only execute this clause once. After that, we simplify the bound
                    // extensions to prevent combinatorial explosion.
                    equalityOnly = false;

                    BoundBuilders newBuilders;
                    BoundBuilders::const_iterator i;
                    for( i = builders.begin(); i != builders.end(); ++i ) {
                        BSONObj first = i->first->obj();
                        BSONObj second = i->second->obj();

                        for(BoundListIter j = fieldBounds.begin(); j != fieldBounds.end(); ++j ) {
                            uassert( 16452,
                                     "combinatorial limit of $in partitioning of results exceeded" ,
                                     newBuilders.size() < MAX_IN_COMBINATIONS );
                            newBuilders.push_back(
                                     make_pair( shared_ptr<BSONObjBuilder>( new BSONObjBuilder() ),
                                                shared_ptr<BSONObjBuilder>( new BSONObjBuilder())));
                            newBuilders.back().first->appendElements( first );
                            newBuilders.back().second->appendElements( second );
                            newBuilders.back().first->appendElements( j->first );
                            newBuilders.back().second->appendElements( j->second );
                        }
                    }
                    builders = newBuilders;
                }
            }
            else {
                // if we've already generated a range or multiple point-intervals
                // just extend what we've generated with min/max bounds for this field
                BoundBuilders::const_iterator j;
                for( j = builders.begin(); j != builders.end(); ++j ) {
                    j->first->appendElements( fieldBounds.front().first );
                    j->second->appendElements( fieldBounds.back().second );
                }
            }
        }
        BoundList ret;
        for( BoundBuilders::const_iterator i = builders.begin(); i != builders.end(); ++i )
            ret.push_back( make_pair( i->first->obj(), i->second->obj() ) );
        return ret;
    }

    BoundList KeyPattern::_transformFieldBounds( const vector<FieldInterval>& oldIntervals ,
                                                 const BSONElement& field  ) const {

        BoundList ret;
        vector<FieldInterval>::const_iterator i;
        for( i = oldIntervals.begin(); i != oldIntervals.end(); ++i ) {
            if ( isAscending( field ) ){
                // straightforward map [a,b] --> [a,b]
                ret.push_back( make_pair( BSON( field.fieldName() << i->_lower._bound ) ,
                                          BSON( field.fieldName() << i->_upper._bound ) ) );
            } else if ( isDescending( field ) ) {
                // reverse [a,b] --> [b,a]
                ret.push_back( make_pair( BSON( field.fieldName() << i->_upper._bound ) ,
                                          BSON( field.fieldName() << i->_lower._bound ) ) );
            } else if ( isHashed( field ) ){
                if ( i->equality() ) {
                    // hash [a,a] --> [hash(a),hash(a)]
                    long long int h = BSONElementHasher::hash64( i->_lower._bound ,
                                                             BSONElementHasher::DEFAULT_HASH_SEED );
                    ret.push_back( make_pair( BSON( field.fieldName() << h ) ,
                                              BSON( field.fieldName() << h ) ) );
                } else {
                    // if it's a range interval and this field is hashed, just generate one
                    // big interval from MinKey to MaxKey, since these vals could lie anywhere
                    ret.clear();
                    ret.push_back( make_pair( BSON( field.fieldName() << MINKEY ) ,
                                              BSON( field.fieldName() << MAXKEY ) ) );
                    break;
                }
            }
        }

        if ( isDescending( field ) ) {
            // now order is [ [2,1], [4,3] , [6,5]....[n,n-1] ].  Reverse to get decreasing order.
            reverse( ret.begin() , ret.end() );
        } else if ( isHashed( field ) ){
            // [ hash(a) , hash(b) , hash(c) ...] no longer in order, so sort before returning
            sort( ret.begin() , ret.end() );
        }

        return ret;
    }

} // namespace mongo
