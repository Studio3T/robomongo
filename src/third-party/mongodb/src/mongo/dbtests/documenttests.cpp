// documenttests.cpp : Unit tests for Document, Value, and related classes.

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

#include "pch.h"

#include "mongo/db/pipeline/document.h"
#include "mongo/db/pipeline/field_path.h"
#include "mongo/db/pipeline/value.h"

#include "dbtests.h"

namespace DocumentTests {

    mongo::Document::FieldPair getNthField(mongo::Document doc, size_t index) {
        mongo::FieldIterator it (doc);
        while (index--) // advance index times
            it.next();
        return it.next();
    }

    namespace Document {

        using mongo::Document;

        BSONObj toBson( const Document& document ) {
            BSONObjBuilder bob;
            document->toBson( &bob );
            return bob.obj();
        }

        Document fromBson( BSONObj obj ) {
            return Document::createFromBsonObj( &obj );
        }

        void assertRoundTrips( const Document& document1 ) {
            BSONObj obj1 = toBson( document1 );
            Document document2 = fromBson( obj1 );
            BSONObj obj2 = toBson( document2 );
            ASSERT_EQUALS( obj1, obj2 );
            ASSERT_EQUALS( 0, Document::compare( document1, document2 ) );
        }

        /** Create a Document. */
        class Create {
        public:
            void run() {
                Document document;
                ASSERT_EQUALS( 0U, document->getFieldCount() );
                assertRoundTrips( document );
            }
        };

        /** Create a Document from a BSONObj. */
        class CreateFromBsonObj {
        public:
            void run() {
                Document document = fromBson( BSONObj() );
                ASSERT_EQUALS( 0U, document->getFieldCount() );
                document = fromBson( BSON( "a" << 1 << "b" << "q" ) );
                ASSERT_EQUALS( 2U, document->getFieldCount() );
                ASSERT_EQUALS( "a", getNthField(document, 0).first.toString() );
                ASSERT_EQUALS( 1,   getNthField(document, 0).second.getInt() );
                ASSERT_EQUALS( "b", getNthField(document, 1).first.toString() );
                ASSERT_EQUALS( "q", getNthField(document, 1).second.getString() );
                assertRoundTrips( document );
            }            
        };

        /** Add Document fields. */
        class AddField {
        public:
            void run() {
                MutableDocument md;
                md.addField( "foo", Value::createInt( 1 ) );
                ASSERT_EQUALS( 1U, md.peek().getFieldCount() );
                ASSERT_EQUALS( 1, md.peek().getValue( "foo" ).getInt() );
                md.addField( "bar", Value::createInt( 99 ) );
                ASSERT_EQUALS( 2U, md.peek().getFieldCount() );
                ASSERT_EQUALS( 99, md.peek().getValue( "bar" ).getInt() );
                // No assertion is triggered by a duplicate field name.
                md.addField( "a", Value::createInt( 5 ) );

                Document final = md.freeze();
                ASSERT_EQUALS( 3U, final.getFieldCount() );
                assertRoundTrips( final );
            }
        };

        /** Get Document values. */
        class GetValue {
        public:
            void run() {
                Document document = fromBson( BSON( "a" << 1 << "b" << 2.2 ) );
                ASSERT_EQUALS( 1, document->getValue( "a" ).getInt() );
                ASSERT_EQUALS( 1, document->getField( "a" ).getInt() );
                ASSERT_EQUALS( 2.2, document->getValue( "b" ).getDouble() );
                ASSERT_EQUALS( 2.2, document->getField( "b" ).getDouble() );
                // Missing field.
                ASSERT( document->getValue( "c" ).missing() );
                ASSERT( document->getField( "c" ).missing() );
                assertRoundTrips( document );
            }
        };

        /** Get Document fields. */
        class SetField {
        public:
            void run() {
                Document original = fromBson(BSON("a" << 1 << "b" << 2.2 << "c" << 99));

                // Initial positions. Used at end of function to make sure nothing moved
                const Position apos = original.positionOf("a");
                const Position bpos = original.positionOf("c");
                const Position cpos = original.positionOf("c");

                MutableDocument md (original);

                // Set the first field.
                md.setField( "a" , Value( "foo" ) );
                ASSERT_EQUALS( 3U, md.peek().getFieldCount() );
                ASSERT_EQUALS( "foo", md.peek().getValue( "a" ).getString() );
                ASSERT_EQUALS( "foo", getNthField(md.peek(), 0).second.getString() );
                assertRoundTrips( md.peek() );
                // Set the second field.
                md["b"] = Value("bar");
                ASSERT_EQUALS( 3U, md.peek().getFieldCount() );
                ASSERT_EQUALS( "bar", md.peek().getValue( "b" ).getString() );
                ASSERT_EQUALS( "bar", getNthField(md.peek(), 1).second.getString() );
                assertRoundTrips( md.peek() );

                // Remove the second field.
                md.setField("b", Value());
                PRINT(md.peek().toString());
                ASSERT_EQUALS( 2U, md.peek().getFieldCount() );
                ASSERT( md.peek().getValue( "b" ).missing() );
                ASSERT_EQUALS( "a", getNthField(md.peek(), 0 ).first.toString() );
                ASSERT_EQUALS( "c", getNthField(md.peek(), 1 ).first.toString() );
                ASSERT_EQUALS( 99, md.peek().getValue("c").getInt() );
                assertRoundTrips( md.peek() );

                // Remove the first field.
                md["a"] = Value();
                ASSERT_EQUALS( 1U, md.peek().getFieldCount() );
                ASSERT( md.peek().getValue( "a" ).missing() );
                ASSERT_EQUALS( "c", getNthField(md.peek(), 0 ).first.toString() );
                ASSERT_EQUALS( 99, md.peek().getValue("c").getInt() );
                assertRoundTrips( md.peek() );

                // Remove the final field. Verify document is empty.
                md.remove("c");
                ASSERT( md.peek().empty() );
                ASSERT_EQUALS( 0U, md.peek().getFieldCount() );
                ASSERT_EQUALS( 0, Document::compare(md.peek(), Document()) );
                ASSERT( !FieldIterator(md.peek()).more() );
                ASSERT( md.peek().getValue( "c" ).missing() );
                assertRoundTrips( md.peek() );

                // Make sure nothing moved
                ASSERT_EQUALS(apos, md.peek().positionOf("a"));
                ASSERT_EQUALS(bpos, md.peek().positionOf("c"));
                ASSERT_EQUALS(cpos, md.peek().positionOf("c"));
                ASSERT_EQUALS(Position(), md.peek().positionOf("d"));
            }
        };

        /** Document comparator. */
        class Compare {
        public:
            void run() {
                assertComparison( 0, BSONObj(), BSONObj() );
                assertComparison( 0, BSON( "a" << 1 ), BSON( "a" << 1 ) );
                assertComparison( -1, BSONObj(), BSON( "a" << 1 ) );
                assertComparison( -1, BSON( "a" << 1 ), BSON( "c" << 1 ) );
                assertComparison( 0, BSON( "a" << 1 << "r" << 2 ), BSON( "a" << 1 << "r" << 2 ) );
                assertComparison( -1, BSON( "a" << 1 ), BSON( "a" << 1 << "r" << 2 ) );
                assertComparison( 0, BSON( "a" << 2 ), BSON( "a" << 2 ) );
                assertComparison( -1, BSON( "a" << 1 ), BSON( "a" << 2 ) );
                assertComparison( -1, BSON( "a" << 1 << "b" << 1 ), BSON( "a" << 1 << "b" << 2 ) );
                // numbers sort before strings
                assertComparison( -1, BSON( "a" << 1 ), BSON( "a" << "foo" ) );
            }
        public:
            int cmp( const BSONObj& a, const BSONObj& b ) {
                int result = Document::compare( fromBson( a ), fromBson( b ) );
                return // sign
                    result < 0 ? -1 :
                    result > 0 ? 1 :
                    0;
            }
            void assertComparison( int expectedResult, const BSONObj& a, const BSONObj& b ) {
                ASSERT_EQUALS( expectedResult, cmp( a, b ) );
                ASSERT_EQUALS( -expectedResult, cmp( b, a ) );
                if ( expectedResult == 0 ) {
                    ASSERT_EQUALS( hash( a ), hash( b ) );
                }
            }
            size_t hash( const BSONObj& obj ) {
                size_t seed = 0x106e1e1;
                fromBson( obj )->hash_combine( seed );
                return seed;
            }
        };

        /** Comparison based on a null field's name.  Differs from BSONObj comparison behavior. */
        class CompareNamedNull {
        public:
            void run() {
                BSONObj obj1 = BSON( "z" << BSONNULL );
                BSONObj obj2 = BSON( "a" << 1 );
                // Comparsion with type precedence.
                ASSERT( obj1.woCompare( obj2 ) < 0 );
                // Comparison with field name precedence.
                ASSERT( Document::compare( fromBson( obj1 ), fromBson( obj2 ) ) > 0 );
            }
        };

        /** Shallow copy clone of a single field Document. */
        class Clone {
        public:
            void run() {
                const Document document = fromBson( BSON( "a" << BSON( "b" << 1 ) ) );
                MutableDocument cloneOnDemand (document);

                // Check equality.
                ASSERT_EQUALS( 0, Document::compare( document, cloneOnDemand.peek() ) );
                // Check pointer equality of sub document.
                ASSERT_EQUALS( document->getValue( "a" ).getDocument().getPtr(),
                               cloneOnDemand.peek().getValue( "a" ).getDocument().getPtr() );


                // Change field in clone and ensure the original document's field is unchanged.
                cloneOnDemand.setField( StringData("a"), Value(2) );
                ASSERT_EQUALS( Value(1), document->getNestedField(FieldPath("a.b")) );


                // setNestedField and ensure the original document is unchanged.

                cloneOnDemand.reset(document);
                vector<Position> path;
                ASSERT_EQUALS( Value(1), document->getNestedField(FieldPath("a.b"), &path) );

                cloneOnDemand.setNestedField(path, Value(2));

                ASSERT_EQUALS( Value(1), document.getNestedField(FieldPath("a.b")) );
                ASSERT_EQUALS( Value(2), cloneOnDemand.peek().getNestedField(FieldPath("a.b")) );
                ASSERT_EQUALS( BSON( "a" << BSON( "b" << 1 ) ), toBson( document ) );
                ASSERT_EQUALS( BSON( "a" << BSON( "b" << 2 ) ), toBson( cloneOnDemand.freeze() ) );
            }
        };

        /** Shallow copy clone of a multi field Document. */
        class CloneMultipleFields {
        public:
            void run() {
                Document document =
                        fromBson( fromjson( "{a:1,b:['ra',4],c:{z:1},d:'lal'}" ) );
                Document clonedDocument = document->clone();
                ASSERT_EQUALS( 0, Document::compare( document, clonedDocument ) );
            }
        };

        /** FieldIterator for an empty Document. */
        class FieldIteratorEmpty {
        public:
            void run() {
                FieldIterator iterator ( (Document()) );
                ASSERT( !iterator.more() );
            }
        };

        /** FieldIterator for a single field Document. */
        class FieldIteratorSingle {
        public:
            void run() {
                FieldIterator iterator (fromBson( BSON( "a" << 1 ) ));
                ASSERT( iterator.more() );
                Document::FieldPair field = iterator.next();
                ASSERT_EQUALS( "a", field.first.toString() );
                ASSERT_EQUALS( 1, field.second.getInt() );
                ASSERT( !iterator.more() );
            }
        };
        
        /** FieldIterator for a multiple field Document. */
        class FieldIteratorMultiple {
        public:
            void run() {
                FieldIterator iterator (fromBson( BSON( "a" << 1 << "b" << 5.6 << "c" << "z" )));
                ASSERT( iterator.more() );
                Document::FieldPair field = iterator.next();
                ASSERT_EQUALS( "a", field.first.toString() );
                ASSERT_EQUALS( 1, field.second.getInt() );
                ASSERT( iterator.more() );

                Document::FieldPair field2 = iterator.next();
                ASSERT_EQUALS( "b", field2.first.toString() );
                ASSERT_EQUALS( 5.6, field2.second.getDouble() );
                ASSERT( iterator.more() );

                Document::FieldPair field3 = iterator.next();
                ASSERT_EQUALS( "c", field3.first.toString() );
                ASSERT_EQUALS( "z", field3.second.getString() );
                ASSERT( !iterator.more() );
            }
        };

        class AllTypesDoc {
        public:
            void run() {
                // These are listed in order of BSONType with some duplicates
                append("minkey", MINKEY);
                // EOO not valid in middle of BSONObj
                append("double", 1.0);
                append("c-string", "string\0after NUL"); // after NULL is ignored
                append("c++", StringData("string\0after NUL", StringData::LiteralTag()).toString());
                append("StringData", StringData("string\0after NUL", StringData::LiteralTag()));
                append("emptyObj", BSONObj());
                append("filledObj", BSON("a" << 1));
                append("emptyArray", BSON("" << BSONArray()).firstElement());
                append("filledArray", BSON("" << BSON_ARRAY(1 << "a")).firstElement());
                append("binData", BSONBinData("a\0b", 3, BinDataGeneral));
                append("binDataCustom", BSONBinData("a\0b", 3, bdtCustom));
                append("binDataUUID", BSONBinData("123456789\0abcdef", 16, bdtUUID));
                append("undefined", BSONUndefined);
                append("oid", OID());
                append("true", true);
                append("false", false);
                append("date", jsTime());
                append("null", BSONNULL);
                append("regex", BSONRegEx(".*"));
                append("regexFlags", BSONRegEx(".*", "i"));
                append("regexEmpty", BSONRegEx("", ""));
                append("dbref", BSONDBRef("foo", OID()));
                append("code", BSONCode("function() {}"));
                append("codeNul", BSONCode(StringData("var nul = '\0'", StringData::LiteralTag())));
                append("symbol", BSONSymbol("foo"));
                append("symbolNul", BSONSymbol(StringData("f\0o", StringData::LiteralTag())));
                append("codeWScope", BSONCodeWScope("asdf", BSONObj()));
                append("codeWScopeWScope", BSONCodeWScope("asdf", BSON("one" << 1)));
                append("int", 1);
                append("timestamp", OpTime());
                append("long", 1LL);
                append("very long", 1LL << 40);
                append("maxkey", MAXKEY);

                const BSONArray arr = arrBuilder.arr();

                // can't use append any more since arrBuilder is done
                objBuilder << "mega array" << arr;
                docBuilder["mega array"] = mongo::Value(values);

                const BSONObj obj = objBuilder.obj();
                const Document doc = docBuilder.freeze();

                const BSONObj obj2 = toBson(doc);
                const Document doc2 = fromBson(obj);

                // logical equality
                ASSERT_EQUALS(obj, obj2);
                if (Document::compare(doc, doc2)) {
                    PRINT(doc);
                    PRINT(doc2);
                }
                ASSERT_EQUALS(Document::compare(doc, doc2), 0);

                // binary equality
                ASSERT_EQUALS(obj.objsize(), obj2.objsize());
                ASSERT_EQUALS(memcmp(obj.objdata(), obj2.objdata(), obj.objsize()), 0);
            }

            template <typename T>
            void append(const char* name, const T& thing) {
                objBuilder << name << thing;
                arrBuilder         << thing;
                docBuilder[name] = mongo::Value(thing);
                values.push_back(mongo::Value(thing));
            }

            vector<mongo::Value> values;
            MutableDocument docBuilder;
            BSONObjBuilder objBuilder;
            BSONArrayBuilder arrBuilder;
        };
    } // namespace Document

    namespace Value {

        using mongo::Value;

        BSONObj toBson( const Value& value ) {
            if (value.missing())
                return BSONObj(); // EOO

            BSONObjBuilder bob;
            value.addToBsonObj( &bob, "" );
            return bob.obj();
        }

        Value fromBson( const BSONObj& obj ) {
            BSONElement element = obj.firstElement();
            return Value::createFromBsonElement( &element );
        }

        void assertRoundTrips( const Value& value1 ) {
            BSONObj obj1 = toBson( value1 );
            Value value2 = fromBson( obj1 );
            BSONObj obj2 = toBson( value2 );
            ASSERT_EQUALS( obj1, obj2 );
            ASSERT_EQUALS(value1, value2);
            ASSERT_EQUALS(value1.getType(), value2.getType());
        }

        /** Int type. */
        class Int {
        public:
            void run() {
                Value value = Value::createInt( 5 );
                ASSERT_EQUALS( 5, value.getInt() );
                ASSERT_EQUALS( 5, value.getLong() );
                ASSERT_EQUALS( 5, value.getDouble() );
                ASSERT_EQUALS( NumberInt, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** Long type. */
        class Long {
        public:
            void run() {
                Value value = Value::createLong( 99 );
                ASSERT_EQUALS( 99, value.getLong() );
                ASSERT_EQUALS( 99, value.getDouble() );
                ASSERT_EQUALS( NumberLong, value.getType() );
                assertRoundTrips( value );
            }
        };
        
        /** Double type. */
        class Double {
        public:
            void run() {
                Value value = Value::createDouble( 5.5 );
                ASSERT_EQUALS( 5.5, value.getDouble() );
                ASSERT_EQUALS( NumberDouble, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** String type. */
        class String {
        public:
            void run() {
                Value value = Value::createString( "foo" );
                ASSERT_EQUALS( "foo", value.getString() );
                ASSERT_EQUALS( mongo::String, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** String with a null character. */
        class StringWithNull {
        public:
            void run() {
                string withNull( "a\0b", 3 );
                BSONObj objWithNull = BSON( "" << withNull );
                ASSERT_EQUALS( withNull, objWithNull[ "" ].str() );
                Value value = fromBson( objWithNull );
                ASSERT_EQUALS( withNull, value.getString() );
                assertRoundTrips( value );                
            }
        };

        /** Date type. */
        class Date {
        public:
            void run() {
                Value value = Value::createDate(999);
                ASSERT_EQUALS( 999, value.getDate() );
                ASSERT_EQUALS( mongo::Date, value.getType() );
                assertRoundTrips( value );
            }
        };
        
        /** Timestamp type. */
        class Timestamp {
        public:
            void run() {
                Value value = Value::createTimestamp( OpTime( 777 ) );
                ASSERT( OpTime( 777 ) == value.getTimestamp() );
                ASSERT_EQUALS( mongo::Timestamp, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** Document with no fields. */
        class EmptyDocument {
        public:
            void run() {
                mongo::Document document = mongo::Document();
                Value value = Value::createDocument( document );
                ASSERT_EQUALS( document.getPtr(), value.getDocument().getPtr() );
                ASSERT_EQUALS( Object, value.getType() );                
                assertRoundTrips( value );
            }
        };

        /** Document type. */
        class Document {
        public:
            void run() {
                mongo::MutableDocument md;
                md.addField( "a", Value::createInt( 5 ) );
                md.addField( "apple", Value::createString( "rrr" ) );
                md.addField( "banana", Value::createDouble( -.3 ) );
                mongo::Document document = md.freeze();

                Value value = Value::createDocument( document );
                // Check document pointers are equal.
                ASSERT_EQUALS( document.getPtr(), value.getDocument().getPtr() );
                // Check document contents.
                ASSERT_EQUALS( 5, document->getValue( "a" ).getInt() );
                ASSERT_EQUALS( "rrr", document->getValue( "apple" ).getString() );
                ASSERT_EQUALS( -.3, document->getValue( "banana" ).getDouble() );
                ASSERT_EQUALS( Object, value.getType() );                
                assertRoundTrips( value );
            }
        };
        
        /** Array with no elements. */
        class EmptyArray {
        public:
            void run() {
                vector<Value> array;
                Value value (array);
                const vector<Value>& array2 =  value.getArray();

                ASSERT( array2.empty() );
                ASSERT_EQUALS( Array, value.getType() );
                ASSERT_EQUALS( 0U, value.getArrayLength() );
                assertRoundTrips( value );
            }
        };

        /** Array type. */
        class Array {
        public:
            void run() {
                vector<Value> array;
                array.push_back( Value::createInt( 5 ) );
                array.push_back( Value::createString( "lala" ) );
                array.push_back( Value::createDouble( 3.14 ) );
                Value value = Value::createArray( array );
                const vector<Value>& array2 = value.getArray();

                ASSERT( !array2.empty() );
                ASSERT_EQUALS( array2.size(), 3U);
                ASSERT_EQUALS( 5, array2[0].getInt() );
                ASSERT_EQUALS( "lala", array2[1].getString() );
                ASSERT_EQUALS( 3.14, array2[2].getDouble() );
                ASSERT_EQUALS( mongo::Array, value.getType() );
                ASSERT_EQUALS( 3U, value.getArrayLength() );
                assertRoundTrips( value );
            }
        };

        /** Oid type. */
        class Oid {
        public:
            void run() {
                Value value =
                        fromBson( BSON( "" << OID( "abcdefabcdefabcdefabcdef" ) ) );
                ASSERT_EQUALS( OID( "abcdefabcdefabcdefabcdef" ), value.getOid() );
                ASSERT_EQUALS( jstOID, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** Bool type. */
        class Bool {
        public:
            void run() {
                Value value = fromBson( BSON( "" << true ) );
                ASSERT_EQUALS( true, value.getBool() );
                ASSERT_EQUALS( mongo::Bool, value.getType() );
                assertRoundTrips( value );                
            }
        };

        /** Regex type. */
        class Regex {
        public:
            void run() {
                Value value = fromBson( fromjson( "{'':/abc/}" ) );
                ASSERT_EQUALS( string("abc"), value.getRegex() );
                ASSERT_EQUALS( RegEx, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** Symbol type (currently unsupported). */
        class Symbol {
        public:
            void run() {
                Value value (BSONSymbol("FOOBAR"));
                ASSERT_EQUALS( "FOOBAR", value.getSymbol() );
                ASSERT_EQUALS( mongo::Symbol, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** Undefined type. */
        class Undefined {
        public:
            void run() {
                Value value = Value(mongo::Undefined);
                ASSERT_EQUALS( mongo::Undefined, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** Null type. */
        class Null {
        public:
            void run() {
                Value value = Value(mongo::jstNULL);
                ASSERT_EQUALS( jstNULL, value.getType() );
                assertRoundTrips( value );
            }
        };

        /** True value. */
        class True {
        public:
            void run() {
                Value value = Value(true);
                ASSERT_EQUALS( true, value.getBool() );
                ASSERT_EQUALS( mongo::Bool, value.getType() );
                assertRoundTrips( value );
            }            
        };

        /** False value. */
        class False {
        public:
            void run() {
                Value value = Value(false);
                ASSERT_EQUALS( false, value.getBool() );
                ASSERT_EQUALS( mongo::Bool, value.getType() );
                assertRoundTrips( value );
            }            
        };
        
        /** -1 value. */
        class MinusOne {
        public:
            void run() {
                Value value = Value(-1);
                ASSERT_EQUALS( -1, value.getInt() );
                ASSERT_EQUALS( NumberInt, value.getType() );
                assertRoundTrips( value );
            }
        };
        
        /** 0 value. */
        class Zero {
        public:
            void run() {
                Value value = Value(0);
                ASSERT_EQUALS( 0, value.getInt() );
                ASSERT_EQUALS( NumberInt, value.getType() );
                assertRoundTrips( value );
            }
        };
        
        /** 1 value. */
        class One {
        public:
            void run() {
                Value value = Value(1);
                ASSERT_EQUALS( 1, value.getInt() );
                ASSERT_EQUALS( NumberInt, value.getType() );
                assertRoundTrips( value );
            }
        };

        namespace Coerce {

            class ToBoolBase {
            public:
                virtual ~ToBoolBase() {
                }
                void run() {
                    ASSERT_EQUALS( expected(), value().coerceToBool() );
                }
            protected:
                virtual Value value() = 0;
                virtual bool expected() = 0;
            };

            class ToBoolTrue : public ToBoolBase {
                bool expected() { return true; }
            };
            
            class ToBoolFalse : public ToBoolBase {
                bool expected() { return false; }
            };

            /** Coerce 0 to bool. */
            class ZeroIntToBool : public ToBoolFalse {
                Value value() { return Value::createInt( 0 ); }
            };
            
            /** Coerce -1 to bool. */
            class NonZeroIntToBool : public ToBoolTrue {
                Value value() { return Value::createInt( -1 ); }
            };
            
            /** Coerce 0LL to bool. */
            class ZeroLongToBool : public ToBoolFalse {
                Value value() { return Value::createLong( 0 ); }
            };
            
            /** Coerce 5LL to bool. */
            class NonZeroLongToBool : public ToBoolTrue {
                Value value() { return Value::createLong( 5 ); }
            };
            
            /** Coerce 0.0 to bool. */
            class ZeroDoubleToBool : public ToBoolFalse {
                Value value() { return Value::createDouble( 0 ); }
            };
            
            /** Coerce -1.3 to bool. */
            class NonZeroDoubleToBool : public ToBoolTrue {
                Value value() { return Value::createDouble( -1.3 ); }
            };

            /** Coerce "" to bool. */
            class StringToBool : public ToBoolTrue {
                Value value() { return Value::createString( "" ); }                
            };
            
            /** Coerce {} to bool. */
            class ObjectToBool : public ToBoolTrue {
                Value value() {
                    return Value::createDocument( mongo::Document() );
                }
            };
            
            /** Coerce [] to bool. */
            class ArrayToBool : public ToBoolTrue {
                Value value() {
                    return Value::createArray( vector<Value>() );
                }
            };

            /** Coerce Date(0) to bool. */
            class DateToBool : public ToBoolTrue {
                Value value() { return Value::createDate(0); }
            };
            
            /** Coerce js literal regex to bool. */
            class RegexToBool : public ToBoolTrue {
                Value value() { return fromBson( fromjson( "{''://}" ) ); }
            };
            
            /** Coerce true to bool. */
            class TrueToBool : public ToBoolTrue {
                Value value() { return fromBson( BSON( "" << true ) ); }
            };
            
            /** Coerce false to bool. */
            class FalseToBool : public ToBoolFalse {
                Value value() { return fromBson( BSON( "" << false ) ); }
            };
            
            /** Coerce null to bool. */
            class NullToBool : public ToBoolFalse {
                Value value() { return Value(mongo::jstNULL); }
            };
            
            /** Coerce undefined to bool. */
            class UndefinedToBool : public ToBoolFalse {
                Value value() { return Value(mongo::Undefined); }
            };

            class ToIntBase {
            public:
                virtual ~ToIntBase() {
                }
                void run() {
                    if (asserts())
                        ASSERT_THROWS( value().coerceToInt(), UserException );
                    else
                        ASSERT_EQUALS( expected(), value().coerceToInt() );
                }
            protected:
                virtual Value value() = 0;
                virtual int expected() { return 0; }
                virtual bool asserts() { return false; }
            };

            /** Coerce -5 to int. */
            class IntToInt : public ToIntBase {
                Value value() { return Value::createInt( -5 ); }
                int expected() { return -5; }
            };
            
            /** Coerce long to int. */
            class LongToInt : public ToIntBase {
                Value value() { return Value::createLong( 0xff00000007LL ); }
                int expected() { return 7; }
            };
            
            /** Coerce 9.8 to int. */
            class DoubleToInt : public ToIntBase {
                Value value() { return Value::createDouble( 9.8 ); }
                int expected() { return 9; }
            };
            
            /** Coerce null to int. */
            class NullToInt : public ToIntBase {
                Value value() { return Value(mongo::jstNULL); }
                bool asserts() { return true; }
            };
            
            /** Coerce undefined to int. */
            class UndefinedToInt : public ToIntBase {
                Value value() { return Value(mongo::Undefined); }
                bool asserts() { return true; }
            };
            
            /** Coerce "" to int unsupported. */
            class StringToInt {
            public:
                void run() {
                    ASSERT_THROWS( Value::createString( "" ).coerceToInt(), UserException );
                }
            };
            
            class ToLongBase {
            public:
                virtual ~ToLongBase() {
                }
                void run() {
                    if (asserts())
                        ASSERT_THROWS( value().coerceToLong(), UserException );
                    else
                        ASSERT_EQUALS( expected(), value().coerceToLong() );
                }
            protected:
                virtual Value value() = 0;
                virtual long long expected() { return 0; }
                virtual bool asserts() { return false; }
            };
            
            /** Coerce -5 to long. */
            class IntToLong : public ToLongBase {
                Value value() { return Value::createInt( -5 ); }
                long long expected() { return -5; }
            };
            
            /** Coerce long to long. */
            class LongToLong : public ToLongBase {
                Value value() { return Value::createLong( 0xff00000007LL ); }
                long long expected() { return 0xff00000007LL; }
            };
            
            /** Coerce 9.8 to long. */
            class DoubleToLong : public ToLongBase {
                Value value() { return Value::createDouble( 9.8 ); }
                long long expected() { return 9; }
            };
            
            /** Coerce null to long. */
            class NullToLong : public ToLongBase {
                Value value() { return Value(mongo::jstNULL); }
                bool asserts() { return true; }
            };
            
            /** Coerce undefined to long. */
            class UndefinedToLong : public ToLongBase {
                Value value() { return Value(mongo::Undefined); }
                bool asserts() { return true; }
            };
            
            /** Coerce string to long unsupported. */
            class StringToLong {
            public:
                void run() {
                    ASSERT_THROWS( Value::createString( "" ).coerceToLong(), UserException );
                }
            };
            
            class ToDoubleBase {
            public:
                virtual ~ToDoubleBase() {
                }
                void run() {
                    if (asserts())
                        ASSERT_THROWS( value().coerceToDouble(), UserException );
                    else
                        ASSERT_EQUALS( expected(), value().coerceToDouble() );
                }
            protected:
                virtual Value value() = 0;
                virtual double expected() { return 0; }
                virtual bool asserts() { return false; }
            };
            
            /** Coerce -5 to double. */
            class IntToDouble : public ToDoubleBase {
                Value value() { return Value::createInt( -5 ); }
                double expected() { return -5; }
            };
            
            /** Coerce long to double. */
            class LongToDouble : public ToDoubleBase {
                Value value() {
                    // A long that cannot be exactly represented as a double.
                    return Value::createDouble( static_cast<double>( 0x8fffffffffffffffLL ) );
                }
                double expected() { return static_cast<double>( 0x8fffffffffffffffLL ); }
            };
            
            /** Coerce double to double. */
            class DoubleToDouble : public ToDoubleBase {
                Value value() { return Value::createDouble( 9.8 ); }
                double expected() { return 9.8; }
            };
            
            /** Coerce null to double. */
            class NullToDouble : public ToDoubleBase {
                Value value() { return Value(mongo::jstNULL); }
                bool asserts() { return true; }
            };
            
            /** Coerce undefined to double. */
            class UndefinedToDouble : public ToDoubleBase {
                Value value() { return Value(mongo::Undefined); }
                bool asserts() { return true; }
            };
            
            /** Coerce string to double unsupported. */
            class StringToDouble {
            public:
                void run() {
                    ASSERT_THROWS( Value::createString( "" ).coerceToDouble(), UserException );
                }
            };

            class ToDateBase {
            public:
                virtual ~ToDateBase() {
                }
                void run() {
                    ASSERT_EQUALS( expected(), value().coerceToDate() );
                }
            protected:
                virtual Value value() = 0;
                virtual long long expected() = 0;
            };

            /** Coerce date to date. */
            class DateToDate : public ToDateBase {
                Value value() { return Value::createDate(888); }
                long long expected() { return 888; }
            };

            /**
             * Convert timestamp to date.  This extracts the time portion of the timestamp, which
             * is different from BSON behavior of interpreting all bytes as a date.
             */
            class TimestampToDate : public ToDateBase {
                Value value() {
                    return Value::createTimestamp( OpTime( 777, 666 ) );
                }
                long long expected() { return 777 * 1000; }
            };
            
            /** Coerce string to date unsupported. */
            class StringToDate {
            public:
                void run() {
                    ASSERT_THROWS( Value::createString( "" ).coerceToDate(), UserException );
                }
            };
            
            class ToStringBase {
            public:
                virtual ~ToStringBase() {
                }
                void run() {
                    ASSERT_EQUALS( expected(), value().coerceToString() );
                }
            protected:
                virtual Value value() = 0;
                virtual string expected() { return ""; }
            };

            /** Coerce -0.2 to string. */
            class DoubleToString : public ToStringBase {
                Value value() { return Value::createDouble( -0.2 ); }
                string expected() { return "-0.2"; }
            };
            
            /** Coerce -4 to string. */
            class IntToString : public ToStringBase {
                Value value() { return Value::createInt( -4 ); }
                string expected() { return "-4"; }
            };
            
            /** Coerce 10000LL to string. */
            class LongToString : public ToStringBase {
                Value value() { return Value::createLong( 10000 ); }
                string expected() { return "10000"; }
            };
            
            /** Coerce string to string. */
            class StringToString : public ToStringBase {
                Value value() { return Value::createString( "fO_o" ); }
                string expected() { return "fO_o"; }
            };
            
            /** Coerce timestamp to string. */
            class TimestampToString : public ToStringBase {
                Value value() {
                    return Value::createTimestamp( OpTime( 1, 2 ) );
                }
                string expected() { return OpTime( 1, 2 ).toStringPretty(); }
            };
            
            /** Coerce date to string. */
            class DateToString : public ToStringBase {
                Value value() { return Value::createDate(1234567890LL*1000); }
                string expected() { return "2009-02-13T23:31:30"; } // from js
            };

            /** Coerce null to string. */
            class NullToString : public ToStringBase {
                Value value() { return Value(mongo::jstNULL); }
            };

            /** Coerce undefined to string. */
            class UndefinedToString : public ToStringBase {
                Value value() { return Value(mongo::Undefined); }
            };

            /** Coerce document to string unsupported. */
            class DocumentToString {
            public:
                void run() {
                    ASSERT_THROWS( Value::createDocument
                                        ( mongo::Document() ).coerceToString(),
                                   UserException );
                }
            };

            /** Coerce timestamp to timestamp. */
            class TimestampToTimestamp {
            public:
                void run() {
                    Value value = Value::createTimestamp( OpTime( 1010 ) );
                    ASSERT( OpTime( 1010 ) == value.coerceToTimestamp() );
                }
            };

            /** Coerce date to timestamp unsupported. */
            class DateToTimestamp {
            public:
                void run() {
                    ASSERT_THROWS( Value::createDate(1010).coerceToTimestamp(),
                                   UserException );
                }
            };

        } // namespace Coerce

        /** Get the "widest" of two numeric types. */
        class GetWidestNumeric {
        public:
            void run() {
                using mongo::Undefined;
                
                // Numeric types.
                assertWidest( NumberInt, NumberInt, NumberInt );
                assertWidest( NumberLong, NumberInt, NumberLong );
                assertWidest( NumberDouble, NumberInt, NumberDouble );
                assertWidest( NumberLong, NumberLong, NumberLong );
                assertWidest( NumberDouble, NumberLong, NumberDouble );
                assertWidest( NumberDouble, NumberDouble, NumberDouble );
                
                // Missing value and numeric types (result Undefined).
                assertWidest( Undefined, NumberInt, Undefined );
                assertWidest( Undefined, NumberInt, Undefined );
                assertWidest( Undefined, NumberLong, jstNULL );
                assertWidest( Undefined, NumberLong, Undefined );
                assertWidest( Undefined, NumberDouble, jstNULL );
                assertWidest( Undefined, NumberDouble, Undefined );

                // Missing value types (result Undefined).
                assertWidest( Undefined, jstNULL, jstNULL );
                assertWidest( Undefined, jstNULL, Undefined );
                assertWidest( Undefined, Undefined, Undefined );

                // Other types (result Undefined).
                assertWidest( Undefined, NumberInt, mongo::Bool );
                assertWidest( Undefined, mongo::String, NumberDouble );
            }
        private:
            void assertWidest( BSONType expectedWidest, BSONType a, BSONType b ) {
                ASSERT_EQUALS( expectedWidest, Value::getWidestNumeric( a, b ) );
                ASSERT_EQUALS( expectedWidest, Value::getWidestNumeric( b, a ) );
            }
        };

        /** Add a Value to a BSONObj. */
        class AddToBsonObj {
        public:
            void run() {
                BSONObjBuilder bob;
                Value::createDouble( 4.4 ).addToBsonObj( &bob, "a" );
                Value::createInt( 22 ).addToBsonObj( &bob, "b" );
                Value::createString( "astring" ).addToBsonObj( &bob, "c" );
                ASSERT_EQUALS( BSON( "a" << 4.4 << "b" << 22 << "c" << "astring" ), bob.obj() );
            }
        };
        
        /** Add a Value to a BSONArray. */
        class AddToBsonArray {
        public:
            void run() {
                BSONArrayBuilder bab;
                Value::createDouble( 4.4 ).addToBsonArray( &bab );
                Value::createInt( 22 ).addToBsonArray( &bab );
                Value::createString( "astring" ).addToBsonArray( &bab );
                ASSERT_EQUALS( BSON_ARRAY( 4.4 << 22 << "astring" ), bab.arr() );
            }
        };

        /** Value comparator. */
        class Compare {
        public:
            void run() {
                BSONObjBuilder undefinedBuilder;
                undefinedBuilder.appendUndefined( "" );
                BSONObj undefined = undefinedBuilder.obj();

                // Undefined / null.
                assertComparison( 0, undefined, undefined );
                assertComparison( -1, undefined, BSON( "" << BSONNULL ) );
                assertComparison( 0, BSON( "" << BSONNULL ), BSON( "" << BSONNULL ) );

                // Undefined / null with other types.
                assertComparison( -1, undefined, BSON( "" << 1 ) );
                assertComparison( -1, undefined, BSON( "" << "bar" ) );
                assertComparison( -1, BSON( "" << BSONNULL ), BSON( "" << -1 ) );
                assertComparison( -1, BSON( "" << BSONNULL ), BSON( "" << "bar" ) );

                // Numeric types.
                assertComparison( 0, 5, 5LL );
                assertComparison( 0, -2, -2.0 );
                assertComparison( 0, 90LL, 90.0 );
                assertComparison( -1, 5, 6LL );
                assertComparison( -1, -2, 2.1 );
                assertComparison( 1, 90LL, 89.999 );
                assertComparison( -1, 90, 90.1 );
                assertComparison( 0, numeric_limits<double>::quiet_NaN(),
                                  numeric_limits<double>::signaling_NaN() );
                assertComparison( -1, numeric_limits<double>::quiet_NaN(), 5 );

                // strings compare between numbers and objects
                assertComparison( 1, "abc", 90 );
                assertComparison( -1, "abc", BSON( "a" << "b" ) );

                // String comparison.
                assertComparison( -1, "", "a" );
                assertComparison( 0, "a", "a" );
                assertComparison( -1, "a", "b" );
                assertComparison( -1, "aa", "b" );
                assertComparison( 1, "bb", "b" );
                assertComparison( 1, "bb", "b" );
                assertComparison( 1, "b-", "b" );
                assertComparison( -1, "b-", "ba" );
                // With a null character.
                assertComparison( 1, string( "a\0", 2 ), "a" );

                // Object.
                assertComparison( 0, fromjson( "{'':{}}" ), fromjson( "{'':{}}" ) );
                assertComparison( 0, fromjson( "{'':{x:1}}" ), fromjson( "{'':{x:1}}" ) );
                assertComparison( -1, fromjson( "{'':{}}" ), fromjson( "{'':{x:1}}" ) );

                // Array.
                assertComparison( 0, fromjson( "{'':[]}" ), fromjson( "{'':[]}" ) );
                assertComparison( -1, fromjson( "{'':[0]}" ), fromjson( "{'':[1]}" ) );
                assertComparison( -1, fromjson( "{'':[0,0]}" ), fromjson( "{'':[1]}" ) );
                assertComparison( -1, fromjson( "{'':[0]}" ), fromjson( "{'':[0,0]}" ) );
                assertComparison( -1, fromjson( "{'':[0]}" ), fromjson( "{'':['']}" ) );

                // OID.
                assertComparison( 0, OID( "abcdefabcdefabcdefabcdef" ),
                                  OID( "abcdefabcdefabcdefabcdef" ) );
                assertComparison( 1, OID( "abcdefabcdefabcdefabcdef" ),
                                  OID( "010101010101010101010101" ) );

                // Bool.
                assertComparison( 0, true, true );
                assertComparison( 0, false, false );
                assertComparison( 1, true, false );

                // Date.
                assertComparison( 0, Date_t( 555 ), Date_t( 555 ) );
                assertComparison( 1, Date_t( 555 ), Date_t( 554 ) );
                // Negative date.
                assertComparison( 1, Date_t( 0 ), Date_t( -1 ) );

                // Regex.
                assertComparison( 0, fromjson( "{'':/a/}" ), fromjson( "{'':/a/}" ) );
                assertComparison( -1, fromjson( "{'':/a/}" ), fromjson( "{'':/a/i}" ) );
                assertComparison( -1, fromjson( "{'':/a/}" ), fromjson( "{'':/aa/}" ) );

                // Timestamp.
                assertComparison( 0, OpTime( 1234 ), OpTime( 1234 ) );
                assertComparison( -1, OpTime( 4 ), OpTime( 1234 ) );

                // Cross-type comparisons. Listed in order of canonical types.
                assertComparison(-1, Value(mongo::MINKEY), Value());
                assertComparison(0,  Value(), Value(mongo::EOO));
                assertComparison(0,  Value(), Value(mongo::Undefined));
                assertComparison(-1, Value(mongo::Undefined), Value(mongo::jstNULL));
                assertComparison(-1, Value(mongo::jstNULL), Value(1));
                assertComparison(0,  Value(1), Value(1LL));
                assertComparison(0,  Value(1), Value(1.0));
                assertComparison(-1, Value(1), Value("string"));
                assertComparison(0,  Value("string"), Value(BSONSymbol("string")));
                assertComparison(-1, Value("string"), Value(mongo::Document()));
                assertComparison(-1, Value(mongo::Document()), Value(mongo::Array));
                assertComparison(-1, Value(mongo::Array), Value(BSONBinData("", 0, MD5Type)));
                assertComparison(-1, Value(BSONBinData("", 0, MD5Type)), Value(mongo::OID()));
                assertComparison(-1, Value(mongo::OID()), Value(false));
                assertComparison(-1, Value(false), Value(OpTime()));
                assertComparison(0,  Value(OpTime()), Value(Date_t(0)));
                assertComparison(-1, Value(Date_t(0)), Value(BSONRegEx("")));
                assertComparison(-1, Value(BSONRegEx("")), Value(BSONDBRef("", mongo::OID())));
                assertComparison(-1, Value(BSONDBRef("", mongo::OID())), Value(BSONCode("")));
                assertComparison(-1, Value(BSONCode("")), Value(BSONCodeWScope("", BSONObj())));
                assertComparison(-1, Value(BSONCodeWScope("", BSONObj())), Value(mongo::MAXKEY));
            }
        private:
            template<class T,class U>
            void assertComparison( int expectedResult, const T& a, const U& b ) {
                assertComparison( expectedResult, BSON( "" << a ), BSON( "" << b ) );
            }
            void assertComparison( int expectedResult, const OpTime& a, const OpTime& b ) {
                BSONObjBuilder first;
                first.appendTimestamp( "", a.asDate() );
                BSONObjBuilder second;
                second.appendTimestamp( "", b.asDate() );
                assertComparison( expectedResult, first.obj(), second.obj() );
            }
            int sign(int cmp) {
                if (cmp == 0) return 0;
                else if (cmp < 0) return -1;
                else return 1;
            }
            int cmp( const Value& a, const Value& b ) {
                return sign(Value::compare(a, b));
            }
            void assertComparison( int expectedResult, const BSONObj& a, const BSONObj& b ) {
                assertComparison(expectedResult, fromBson(a), fromBson(b));
            }
            void assertComparison(int expectedResult, const Value& a, const Value& b) {
                log() << "testing " << a.toString() << " and " << b.toString() << endl;
                // reflexivity
                ASSERT_EQUALS(0, cmp(a, a));
                ASSERT_EQUALS(0, cmp(b, b));

                // symmetry
                ASSERT_EQUALS( expectedResult, cmp( a, b ) );
                ASSERT_EQUALS( -expectedResult, cmp( b, a ) );

                // equal values must hash equally.
                if ( expectedResult == 0 ) {
                    ASSERT_EQUALS( hash( a ), hash( b ) );
                }
                
                // same as BSON
                ASSERT_EQUALS(expectedResult, sign(toBson(a).firstElement().woCompare(
                                                   toBson(b).firstElement())));
            }
            size_t hash(const Value& v) {
                size_t seed = 0xf00ba6;
                v.hash_combine( seed );
                return seed;
            }
        };

        class SubFields {
        public:
            void run() {
                const Value val = fromBson(fromjson(
                            "{'': {a: [{x:1, b:[1, {y:1, c:1234, z:1}, 1]}]}}"));
                           // ^ this outer object is removed by fromBson

                ASSERT(val.getType() == mongo::Object);

                ASSERT(val[999].missing());
                ASSERT(val["missing"].missing());
                ASSERT(val["a"].getType() == mongo::Array);

                ASSERT(val["a"][999].missing());
                ASSERT(val["a"]["missing"].missing());
                ASSERT(val["a"][0].getType() == mongo::Object);

                ASSERT(val["a"][0][999].missing());
                ASSERT(val["a"][0]["missing"].missing());
                ASSERT(val["a"][0]["b"].getType() == mongo::Array);

                ASSERT(val["a"][0]["b"][999].missing());
                ASSERT(val["a"][0]["b"]["missing"].missing());
                ASSERT(val["a"][0]["b"][1].getType() == mongo::Object);

                ASSERT(val["a"][0]["b"][1][999].missing());
                ASSERT(val["a"][0]["b"][1]["missing"].missing());
                ASSERT(val["a"][0]["b"][1]["c"].getType() == mongo::NumberInt);
                ASSERT_EQUALS(val["a"][0]["b"][1]["c"].getInt(), 1234);
            }
        };
        
    } // namespace Value

    class All : public Suite {
    public:
        All() : Suite( "document" ) {
        }
        void setupTests() {
            add<Document::Create>();
            add<Document::CreateFromBsonObj>();
            add<Document::AddField>();
            add<Document::GetValue>();
            add<Document::SetField>();
            add<Document::Compare>();
            add<Document::CompareNamedNull>();
            add<Document::Clone>();
            add<Document::CloneMultipleFields>();
            add<Document::FieldIteratorEmpty>();
            add<Document::FieldIteratorSingle>();
            add<Document::FieldIteratorMultiple>();
            add<Document::AllTypesDoc>();

            add<Value::Int>();
            add<Value::Long>();
            add<Value::Double>();
            add<Value::String>();
            add<Value::StringWithNull>();
            add<Value::Date>();
            add<Value::Timestamp>();
            add<Value::EmptyDocument>();
            add<Value::EmptyArray>();
            add<Value::Array>();
            add<Value::Oid>();
            add<Value::Bool>();
            add<Value::Regex>();
            add<Value::Symbol>();
            add<Value::Undefined>();
            add<Value::Null>();
            add<Value::True>();
            add<Value::False>();
            add<Value::MinusOne>();
            add<Value::Zero>();
            add<Value::One>();

            add<Value::Coerce::ZeroIntToBool>();
            add<Value::Coerce::NonZeroIntToBool>();
            add<Value::Coerce::ZeroLongToBool>();
            add<Value::Coerce::NonZeroLongToBool>();
            add<Value::Coerce::ZeroDoubleToBool>();
            add<Value::Coerce::NonZeroDoubleToBool>();
            add<Value::Coerce::StringToBool>();
            add<Value::Coerce::ObjectToBool>();
            add<Value::Coerce::ArrayToBool>();
            add<Value::Coerce::DateToBool>();
            add<Value::Coerce::RegexToBool>();
            add<Value::Coerce::TrueToBool>();
            add<Value::Coerce::FalseToBool>();
            add<Value::Coerce::NullToBool>();
            add<Value::Coerce::UndefinedToBool>();
            add<Value::Coerce::IntToInt>();
            add<Value::Coerce::LongToInt>();
            add<Value::Coerce::DoubleToInt>();
            add<Value::Coerce::NullToInt>();
            add<Value::Coerce::UndefinedToInt>();
            add<Value::Coerce::StringToInt>();
            add<Value::Coerce::IntToLong>();
            add<Value::Coerce::LongToLong>();
            add<Value::Coerce::DoubleToLong>();
            add<Value::Coerce::NullToLong>();
            add<Value::Coerce::UndefinedToLong>();
            add<Value::Coerce::StringToLong>();
            add<Value::Coerce::IntToDouble>();
            add<Value::Coerce::LongToDouble>();
            add<Value::Coerce::DoubleToDouble>();
            add<Value::Coerce::NullToDouble>();
            add<Value::Coerce::UndefinedToDouble>();
            add<Value::Coerce::StringToDouble>();
            add<Value::Coerce::DateToDate>();
            add<Value::Coerce::TimestampToDate>();
            add<Value::Coerce::StringToDate>();
            add<Value::Coerce::DoubleToString>();
            add<Value::Coerce::IntToString>();
            add<Value::Coerce::LongToString>();
            add<Value::Coerce::StringToString>();
            add<Value::Coerce::TimestampToString>();
            add<Value::Coerce::DateToString>();
            add<Value::Coerce::NullToString>();
            add<Value::Coerce::UndefinedToString>();
            add<Value::Coerce::DocumentToString>();
            add<Value::Coerce::TimestampToTimestamp>();
            add<Value::Coerce::DateToTimestamp>();

            add<Value::GetWidestNumeric>();
            add<Value::AddToBsonObj>();
            add<Value::AddToBsonArray>();
            add<Value::Compare>();
            add<Value::SubFields>();
        }
    } myall;
    
} // namespace DocumentTests
