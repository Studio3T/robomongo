// namespacestring_test.cpp

/*    Copyright 2012 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "mongo/unittest/unittest.h"

#include "mongo/db/namespacestring.h"

namespace mongo {


    TEST( NamespaceStringTest, DatabaseValidNames ) {
        ASSERT( NamespaceString::validDBName( "foo" ) );
        ASSERT( !NamespaceString::validDBName( "foo/bar" ) );
        ASSERT( !NamespaceString::validDBName( "foo bar" ) );
        ASSERT( !NamespaceString::validDBName( "foo.bar" ) );
        
        ASSERT( NamespaceString::normal( "asdads" ) );
        ASSERT( !NamespaceString::normal( "asda$ds" ) );
        ASSERT( NamespaceString::normal( "local.oplog.$main" ) );
    }

    TEST( NamespaceStringTest, DBHash ) {
        ASSERT_EQUALS( nsDBHash( "foo" ), nsDBHash( "foo" ) );
        ASSERT_EQUALS( nsDBHash( "foo" ), nsDBHash( "foo.a" ) );
        ASSERT_EQUALS( nsDBHash( "foo" ), nsDBHash( "foo." ) );

        ASSERT_EQUALS( nsDBHash( "" ), nsDBHash( "" ) );
        ASSERT_EQUALS( nsDBHash( "" ), nsDBHash( ".a" ) );
        ASSERT_EQUALS( nsDBHash( "" ), nsDBHash( "." ) );
        
        ASSERT_NOT_EQUALS( nsDBHash( "foo" ), nsDBHash( "food" ) );
        ASSERT_NOT_EQUALS( nsDBHash( "foo." ), nsDBHash( "food" ) );
        ASSERT_NOT_EQUALS( nsDBHash( "foo.d" ), nsDBHash( "food" ) );
    }

#define testEqualsBothWays(X,Y) ASSERT_TRUE( nsDBEquals( (X), (Y) ) ); ASSERT_TRUE( nsDBEquals( (Y), (X) ) );
#define testNotEqualsBothWays(X,Y) ASSERT_FALSE( nsDBEquals( (X), (Y) ) ); ASSERT_FALSE( nsDBEquals( (Y), (X) ) );

    TEST( NamespaceStringTest, DBEquals ) {
        testEqualsBothWays( "foo" , "foo" );
        testEqualsBothWays( "foo" , "foo.a" );
        testEqualsBothWays( "foo.a" , "foo.a" );
        testEqualsBothWays( "foo.a" , "foo.b" );

        testEqualsBothWays( "" , "" );
        testEqualsBothWays( "" , "." );
        testEqualsBothWays( "" , ".x" );

        testNotEqualsBothWays( "foo" , "bar" );
        testNotEqualsBothWays( "foo" , "food" );
        testNotEqualsBothWays( "foo." , "food" );

        testNotEqualsBothWays( "" , "x" );
        testNotEqualsBothWays( "" , "x." );
        testNotEqualsBothWays( "" , "x.y" );
        testNotEqualsBothWays( "." , "x" );
        testNotEqualsBothWays( "." , "x." );
        testNotEqualsBothWays( "." , "x.y" );
    }

    TEST( NamespaceStringTest, nsToDatabase1 ) {
        ASSERT_EQUALS( "foo", nsToDatabaseSubstring( "foo.bar" ) );
        ASSERT_EQUALS( "foo", nsToDatabaseSubstring( "foo" ) );
        ASSERT_EQUALS( "foo", nsToDatabase( "foo.bar" ) );
        ASSERT_EQUALS( "foo", nsToDatabase( "foo" ) );
        ASSERT_EQUALS( "foo", nsToDatabase( string("foo.bar") ) );
        ASSERT_EQUALS( "foo", nsToDatabase( string("foo") ) );
    }

    TEST( NamespaceStringTest, nsToDatabase2 ) {
        char buf[128];

        nsToDatabase( "foo.bar", buf );
        ASSERT_EQUALS( 'f', buf[0] );
        ASSERT_EQUALS( 'o', buf[1] );
        ASSERT_EQUALS( 'o', buf[2] );
        ASSERT_EQUALS( 0, buf[3] );

        nsToDatabase( "bar", buf );
        ASSERT_EQUALS( 'b', buf[0] );
        ASSERT_EQUALS( 'a', buf[1] );
        ASSERT_EQUALS( 'r', buf[2] );
        ASSERT_EQUALS( 0, buf[3] );


    }
}

