// sm_db.cpp

/*    Copyright 2009 10gen Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
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

//#include <third_party/js-1.7/jsapi.h>

#include "mongo/base/init.h"
#include "mongo/client/dbclientcursor.h"
#include "mongo/db/namespacestring.h"
#include "mongo/scripting/engine_spidermonkey.h"
#include "mongo/scripting/engine_spidermonkey_internal.h"
#include "mongo/util/base64.h"
#include "mongo/util/text.h"

#if( BOOST_VERSION >= 104200 )
#define HAVE_UUID 1
#endif

namespace mongo {

    bool haveLocalShardingInfo( const string& ns );

    // Generated symbols for JS files
    namespace JSFiles {
        extern const JSFile types;
        extern const JSFile assert;
    }

namespace spidermonkey {

    JSFunctionSpec bson_functions[] = {
        { 0 }
    };

    JSBool bson_cons( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        JS_ReportError( cx , "can't construct bson object" );
        return JS_FALSE;
    }

    // ------------    some defs needed ---------------

    JSObject * doCreateCollection( JSContext * cx , JSObject * db , const string& shortName );

    // ------------     utils          ------------------

    bool isSpecialName( const string& name ) {
        static set<string> names;
        if ( names.size() == 0 ) {
            names.insert( "tojson" );
            names.insert( "toJson" );
            names.insert( "toString" );
        }

        if ( name.length() == 0 )
            return false;

        if ( name[0] == '_' )
            return true;

        return names.count( name ) > 0;
    }


    // ------ cursor ------

    class CursorHolder {
    public:
        CursorHolder( auto_ptr< DBClientCursor > &cursor, const shared_ptr< DBClientWithCommands > &connection ) :
            connection_( connection ),
            cursor_( cursor ) {
            verify( cursor_.get() );
        }
        DBClientCursor *get() const { return cursor_.get(); }
    private:
        shared_ptr< DBClientWithCommands > connection_;
        auto_ptr< DBClientCursor > cursor_;
    };

    DBClientCursor *getCursor( JSContext *cx, JSObject *obj ) {
        CursorHolder * holder = (CursorHolder*)JS_GetPrivate( cx , obj );
        uassert( 10235 ,  "no cursor!" , holder );
        return holder->get();
    }

    JSBool internal_cursor_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            uassert( 10236 ,  "no args to internal_cursor_constructor" , argc == 0 );
            verify( JS_SetPrivate( cx , obj , 0 ) ); // just for safety
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16288 );
        }
        return JS_TRUE;
    }

    void internal_cursor_finalize( JSContext * cx , JSObject * obj ) {
        try {
            CursorHolder * holder = (CursorHolder*)JS_GetPrivate( cx , obj );
            if ( holder ) {
                delete holder;
                verify( JS_SetPrivate( cx , obj , 0 ) );
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16289 );
        }
    }

    JSBool internal_cursor_hasNext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            DBClientCursor *cursor = getCursor( cx, obj );
            *rval = cursor->more() ? JSVAL_TRUE : JSVAL_FALSE;
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16290 );
        }
        return JS_TRUE;
    }

    JSBool internal_cursor_objsLeftInBatch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            DBClientCursor *cursor = getCursor( cx, obj );
            Convertor c(cx);
            *rval = c.toval((double) cursor->objsLeftInBatch() );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16291 );
        }
        return JS_TRUE;
    }

    JSBool internal_cursor_next(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            DBClientCursor *cursor = getCursor( cx, obj );
            if ( ! cursor->more() ) {
                JS_ReportError( cx , "cursor at the end" );
                return JS_FALSE;
            }

            BSONObj n = cursor->next();
            Convertor c(cx);
            *rval = c.toval( &n );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16292 );
        }
        return JS_TRUE;
    }

    JSFunctionSpec internal_cursor_functions[] = {
        { "hasNext" , internal_cursor_hasNext , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "objsLeftInBatch" , internal_cursor_objsLeftInBatch , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "next" , internal_cursor_next , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { 0 }
    };

    JSClass internal_cursor_class = {
        "InternalCursor",               // class name
        JSCLASS_HAS_PRIVATE,            // flags
        JS_PropertyStub,                // addProperty
        JS_PropertyStub,                // delProperty
        JS_PropertyStub,                // getProperty
        JS_PropertyStub,                // setProperty
        JS_EnumerateStub,               // enumerate
        JS_ResolveStub,                 // resolve
        JS_ConvertStub,                 // convert
        internal_cursor_finalize,       // finalize
        JSCLASS_NO_OPTIONAL_MEMBERS     // optional members
    };


    // ------ mongo stuff ------

    JSBool mongo_local_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            shared_ptr< DBClientWithCommands > client( createDirectClient() );
            verify( JS_SetPrivate( cx , obj , (void*)( new shared_ptr< DBClientWithCommands >( client ) ) ) );

            Convertor c( cx );
            jsval host = c.toval( "EMBEDDED" );
            verify( JS_SetProperty( cx , obj , "host" , &host ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16293 );
        }
        return JS_TRUE;
    }

    JSBool mongo_external_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            smuassert( cx ,  "0 or 1 args to Mongo" , argc <= 1 );

            string host( "127.0.0.1" );
            Convertor c( cx );
            if ( argc > 0 )
                host = c.toString( argv[0] );

            string errmsg;
            ConnectionString cs = ConnectionString::parse( host , errmsg );
            if ( ! cs.isValid() ) {
                JS_ReportError( cx , errmsg.c_str() );
                return JS_FALSE;
            }

            shared_ptr< DBClientWithCommands > conn( cs.connect( errmsg ) );
            if ( ! conn ) {
                JS_ReportError( cx , errmsg.c_str() );
                return JS_FALSE;
            }

            try {
                ScriptEngine::runConnectCallback( *conn );
            }
            catch ( const AssertionException& e ){
                // Can happen if connection goes down while we're starting up here
                // Catch so that we don't get a hard-to-trace segfault from SM
                JS_ReportError( cx, ((string)( str::stream() << "Error during mongo startup." << causedBy( e ) )).c_str() );
                return JS_FALSE;
            }
            catch ( const std::exception& e ) {
                log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
                fassertFailed( 16294 );
            }

            verify( JS_SetPrivate( cx , obj , (void*)( new shared_ptr< DBClientWithCommands >( conn ) ) ) );
            jsval host_val = c.toval( host.c_str() );
            verify( JS_SetProperty( cx , obj , "host" , &host_val ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16295 );
        }
        return JS_TRUE;
    }

    DBClientWithCommands *getConnection( JSContext *cx, JSObject *obj ) {
        shared_ptr< DBClientWithCommands > * connHolder = (shared_ptr< DBClientWithCommands >*)JS_GetPrivate( cx , obj );
        if (!connHolder)
            return NULL;
        return connHolder->get();
    }

    void mongo_finalize( JSContext * cx , JSObject * obj ) {
        try {
            shared_ptr< DBClientWithCommands > * connHolder = (shared_ptr< DBClientWithCommands >*)JS_GetPrivate( cx , obj );
            if ( connHolder ) {
                delete connHolder;
                verify( JS_SetPrivate( cx , obj , 0 ) );
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16296 );
        }
    }

    JSClass mongo_class = {
        "Mongo",                                        // class name
        JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE,      // flags
        JS_PropertyStub,                                // addProperty
        JS_PropertyStub,                                // delProperty
        JS_PropertyStub,                                // getProperty
        JS_PropertyStub,                                // setProperty
        JS_EnumerateStub,                               // enumerate
        JS_ResolveStub,                                 // resolve
        JS_ConvertStub,                                 // convert
        mongo_finalize,                                 // finalize
        JSCLASS_NO_OPTIONAL_MEMBERS                     // optional members
    };

    JSBool mongo_auth(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            smuassert( cx , "mongo_auth needs 3 args" , argc == 3 );
            DBClientWithCommands *conn = getConnection(cx, obj);
            smuassert(cx , "no connection!", conn);

            Convertor c( cx );

            string db = c.toString( argv[0] );
            string username = c.toString( argv[1] );
            string password = c.toString( argv[2] );
            string errmsg = "";

            if ( ! conn->auth( db, username, password, errmsg ) ) {
                JS_ReportError( cx, errmsg.c_str() );
                return JS_FALSE;
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const SocketException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.toString().c_str() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16297 );
        }
        return JS_TRUE;
    }

    JSBool mongo_logout(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            smuassert(context, "mongo_logout needs 1 arg" , argc == 1);
            shared_ptr<DBClientWithCommands>* connHolder =
                    reinterpret_cast<shared_ptr<DBClientWithCommands>*>(
                            JS_GetPrivate(context , obj));
            smuassert(context,  "no connection!", connHolder && connHolder->get());
            DBClientWithCommands *conn = connHolder->get();

            Convertor convertor(context);
            string db = convertor.toString(argv[0]);

            BSONObj ret;
            conn->logout(db, ret);
            *rval = convertor.toval(&ret);
        }
        catch (const AssertionException& e) {
            if (!JS_IsExceptionPending(context)) {
                JS_ReportError(context, e.what());
            }
            return JS_FALSE;
        }
        catch (const SocketException& e) {
            if (!JS_IsExceptionPending(context)) {
                JS_ReportError(context, e.toString().c_str());
            }
            return JS_FALSE;
        }
        catch (const std::exception& e) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed(16396);
        }

        return JS_TRUE;
    }

    JSBool mongo_find(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            smuassert( cx , "mongo_find needs 7 args" , argc == 7 );
            shared_ptr< DBClientWithCommands > * connHolder = (shared_ptr< DBClientWithCommands >*)JS_GetPrivate( cx , obj );
            smuassert( cx ,  "no connection!" , connHolder && connHolder->get() );
            DBClientWithCommands *conn = connHolder->get();

            Convertor c( cx );

            string ns = c.toString( argv[0] );

            BSONObj q = c.toObject( argv[1] );
            BSONObj f = c.toObject( argv[2] );

            int nToReturn = (int) c.toNumber( argv[3] );
            int nToSkip = (int) c.toNumber( argv[4] );
            int batchSize = (int) c.toNumber( argv[5] );
            int options = (int)c.toNumber( argv[6] );

            auto_ptr<DBClientCursor> cursor = conn->query( ns , q , nToReturn , nToSkip , f.nFields() ? &f : 0  , options , batchSize );
            if ( ! cursor.get() ) {
                log() << "query failed : " << ns << " " << q << " to: " << conn->toString() << endl;
                JS_ReportError( cx , "error doing query: failed" );
                return JS_FALSE;
            }
            JSObject * mycursor = JS_NewObject( cx , &internal_cursor_class , 0 , 0 );
            CHECKNEWOBJECT( mycursor, cx, "internal_cursor_class" );
            verify( JS_SetPrivate( cx , mycursor , new CursorHolder( cursor, *connHolder ) ) );
            *rval = OBJECT_TO_JSVAL( mycursor );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const SocketException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.toString().c_str() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16298 );
        }
        return JS_TRUE;
    }

    JSBool mongo_update(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            smuassert( cx ,  "mongo_update needs at least 3 args" , argc >= 3 );
            smuassert( cx ,  "2nd param to update has to be an object" , JSVAL_IS_OBJECT( argv[1] ) );
            smuassert( cx ,  "3rd param to update has to be an object" , JSVAL_IS_OBJECT( argv[2] ) );

            Convertor c( cx );
            if ( c.getBoolean( obj , "readOnly" ) ) {
                JS_ReportError( cx , "js db in read only mode - mongo_update" );
                return JS_FALSE;
            }

            DBClientWithCommands * conn = getConnection( cx, obj );
            uassert( 10245 ,  "no connection!" , conn );

            string ns = c.toString( argv[0] );

            bool upsert = argc > 3 && c.toBoolean( argv[3] );
            bool multi = argc > 4 && c.toBoolean( argv[4] );

            conn->update( ns , c.toObject( argv[1] ) , c.toObject( argv[2] ) , upsert , multi );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const SocketException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.toString().c_str() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16299 );
        }
        return JS_TRUE;
    }

    JSBool mongo_insert(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            smuassert( cx ,  "mongo_insert needs 3 args" , argc == 3 );
            smuassert( cx ,  "2nd param to insert has to be an object" , JSVAL_IS_OBJECT( argv[1] ) );

            Convertor c( cx );
            if ( c.getBoolean( obj , "readOnly" ) ) {
                JS_ReportError( cx , "js db in read only mode - mongo_insert" );
                return JS_FALSE;
            }

            DBClientWithCommands * conn = getConnection( cx, obj );
            uassert( 10248 ,  "no connection!" , conn );

            string ns = c.toString( argv[0] );

            JSObject * insertObj = JSVAL_TO_OBJECT( argv[1] );

            int flags = static_cast<int>( c.toNumber( argv[2] ) );

            if( JS_IsArrayObject( cx, insertObj ) ){
                vector<BSONObj> bos;

                jsuint len;
                JSBool gotLen = JS_GetArrayLength( cx, insertObj, &len );
                smuassert( cx, "could not get length of array", gotLen );

                for( jsuint i = 0; i < len; i++ ){

                    jsval el;
                    JSBool inserted = JS_GetElement( cx, insertObj, i, &el);
                    smuassert( cx, "could not find element in array object", inserted );

                    bos.push_back( c.toObject( el ) );
                }

                conn->insert( ns, bos, flags );
            }
            else {
                BSONObj o = c.toObject( argv[1] );
                // TODO: add _id

                conn->insert( ns , o );
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const SocketException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.toString().c_str() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16300 );
        }
        return JS_TRUE;
    }

    JSBool mongo_remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            smuassert( cx ,  "mongo_remove needs 2 or 3 arguments" , argc == 2 || argc == 3 );
            smuassert( cx ,  "2nd param to insert has to be an object" , JSVAL_IS_OBJECT( argv[1] ) );

            Convertor c( cx );
            if ( c.getBoolean( obj , "readOnly" ) ) {
                JS_ReportError( cx , "js db in read only mode - mongo_remove" );
                return JS_FALSE;
            }

            DBClientWithCommands * conn = getConnection( cx, obj );
            uassert( 10251 ,  "no connection!" , conn );

            string ns = c.toString( argv[0] );
            BSONObj o = c.toObject( argv[1] );
            bool justOne = false;
            if ( argc > 2 )
                justOne = c.toBoolean( argv[2] );

            conn->remove( ns , o , justOne );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const SocketException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.toString().c_str() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16301 );
        }
        return JS_TRUE;
    }

namespace {
    std::vector<JSFunctionSpec>* mongoFunctionsVector;
    JSFunctionSpec* mongo_functions;

    MONGO_INITIALIZER(SmMongoFunctionsRegistry)(InitializerContext* context) {
        static const JSFunctionSpec mongoFunctionsStandard[] = {
            { "auth" , mongo_auth , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
            { "logout", mongo_logout, 0, JSPROP_READONLY | JSPROP_PERMANENT, 0 },
            { "find" , mongo_find , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
            { "update" , mongo_update , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
            { "insert" , mongo_insert , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
            { "remove" , mongo_remove , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 }
        };
        mongoFunctionsVector = new std::vector<JSFunctionSpec>;
        std::copy(mongoFunctionsStandard,
                  mongoFunctionsStandard + boost::size(mongoFunctionsStandard),
                  std::back_inserter(*mongoFunctionsVector));
        return Status::OK();
    }

    MONGO_INITIALIZER_WITH_PREREQUISITES(
            SmMongoFunctionRegistrationDone, ("SmMongoFunctionsRegistry"))(InitializerContext* cx) {

        static const JSFunctionSpec nullSpec = { 0 };
        mongo_functions = new JSFunctionSpec[mongoFunctionsVector->size() + 1];
        std::copy(mongoFunctionsVector->begin(), mongoFunctionsVector->end(), mongo_functions);
        mongo_functions[mongoFunctionsVector->size()] = nullSpec;
        delete mongoFunctionsVector;
        mongoFunctionsVector = NULL;
        return Status::OK();
    }

}  // namespace

    void registerMongoFunction(const JSFunctionSpec& functionSpec) {
        fassert(16466, mongoFunctionsVector != NULL);
        mongoFunctionsVector->push_back(functionSpec);
    }

    // -------------  db_collection -------------

    JSBool db_collection_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            smuassert( cx ,  "db_collection_constructor wrong args" , argc == 4 );
            Convertor c(cx);
            if ( haveLocalShardingInfo( c.toString( argv[3] ) ) ) {
                JS_ReportError( cx , "can't use sharded collection from db.eval" );
                return JS_FALSE;
            }

            verify( JS_SetProperty( cx , obj , "_mongo" , &(argv[0]) ) );
            verify( JS_SetProperty( cx , obj , "_db" , &(argv[1]) ) );
            verify( JS_SetProperty( cx , obj , "_shortName" , &(argv[2]) ) );
            verify( JS_SetProperty( cx , obj , "_fullName" , &(argv[3]) ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16302 );
        }
        return JS_TRUE;
    }

    JSBool db_collection_resolve( JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **objp ) {
        try {
            if ( flags & JSRESOLVE_ASSIGNING )
                return JS_TRUE;

            Convertor c( cx );
            string collname = c.toString( id );

            if ( isSpecialName( collname ) )
                return JS_TRUE;

            if ( obj == c.getGlobalPrototype( "DBCollection" ) )
                return JS_TRUE;

            JSObject * proto = JS_GetPrototype( cx , obj );
            if ( c.hasProperty( obj , collname.c_str() ) || ( proto && c.hasProperty( proto , collname.c_str() )  ) )
                return JS_TRUE;

            string name = c.toString( c.getProperty( obj , "_shortName" ) );
            name += ".";
            name += collname;

            jsval db = c.getProperty( obj , "_db" );
            if ( ! JSVAL_IS_OBJECT( db ) )
                return JS_TRUE;

            JSObject * coll = doCreateCollection( cx , JSVAL_TO_OBJECT( db ) , name );
            if ( ! coll )
                return JS_FALSE;
            c.setProperty( obj , collname.c_str() , OBJECT_TO_JSVAL( coll ) );
            *objp = obj;
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16303 );
        }
        return JS_TRUE;
    }

    JSClass db_collection_class = {
        "DBCollection",                                 // class name
        JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE,      // flags
        JS_PropertyStub,                                // addProperty
        JS_PropertyStub,                                // delProperty
        JS_PropertyStub,                                // getProperty
        JS_PropertyStub,                                // setProperty
        JS_EnumerateStub,                               // enumerate
        (JSResolveOp)db_collection_resolve,             // resolve
        JS_ConvertStub,                                 // convert
        JS_FinalizeStub,                                // finalize
        JSCLASS_NO_OPTIONAL_MEMBERS                     // optional members
    };


    JSObject * doCreateCollection( JSContext * cx , JSObject * db , const string& shortName ) {
        Convertor c(cx);

        verify( c.hasProperty( db , "_mongo" ) );
        verify( c.hasProperty( db , "_name" ) );

        JSObject * coll = JS_NewObject( cx , &db_collection_class , 0 , 0 );
        CHECKNEWOBJECT( coll, cx, "doCreateCollection" );
        c.setProperty( coll , "_mongo" , c.getProperty( db , "_mongo" ) );
        c.setProperty( coll , "_db" , OBJECT_TO_JSVAL( db ) );
        c.setProperty( coll , "_shortName" , c.toval( shortName.c_str() ) );

        string name = c.toString( c.getProperty( db , "_name" ) );
        name += "." + shortName;
        c.setProperty( coll , "_fullName" , c.toval( name.c_str() ) );

        if ( haveLocalShardingInfo( name ) ) {
            JS_ReportError( cx , "can't use sharded collection from db.eval" );
            return 0;
        }

        return coll;
    }

    // --------------  DB ---------------


    JSBool db_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            smuassert( cx,  "wrong number of arguments to DB" , argc == 2 );

            Convertor convertor( cx );
            string dbName = convertor.toString( argv[1] );
            string msg = str::stream() << "[" << dbName << "] is not a valid database name";
            smuassert( cx, msg.c_str(), NamespaceString::validDBName( dbName ) );

            verify( JS_SetProperty( cx , obj , "_mongo" , &(argv[0]) ) );
            verify( JS_SetProperty( cx , obj , "_name" , &(argv[1]) ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16304 );
        }
        return JS_TRUE;
    }

    JSBool db_resolve( JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **objp ) {
        try {
            if ( flags & JSRESOLVE_ASSIGNING )
                return JS_TRUE;

            Convertor c( cx );

            if ( obj == c.getGlobalPrototype( "DB" ) )
                return JS_TRUE;

            string collname = c.toString( id );

            if ( isSpecialName( collname ) )
                return JS_TRUE;

            JSObject * proto = JS_GetPrototype( cx , obj );
            if ( proto && c.hasProperty( proto , collname.c_str() ) )
                return JS_TRUE;

            JSObject * coll = doCreateCollection( cx , obj , collname );
            if ( ! coll )
                return JS_FALSE;
            c.setProperty( obj , collname.c_str() , OBJECT_TO_JSVAL( coll ) );

            *objp = obj;
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16305 );
        }
        return JS_TRUE;
    }

    JSClass db_class = {
        "DB" , JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, (JSResolveOp)(&db_resolve) , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };


    // -------------- object id -------------

    JSBool object_id_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            Convertor c( cx );

            OID oid;
            if ( argc == 0 ) {
                oid.init();
            }
            else {
                smuassert( cx ,  "object_id_constructor can't take more than 1 param" , argc == 1 );
                string s = c.toString( argv[0] );

                Scope::validateObjectIdString( s );
                oid.init( s );
            }

            if ( ! JS_InstanceOf( cx , obj , &object_id_class , 0 ) ) {
                obj = JS_NewObject( cx , &object_id_class , 0 , 0 );
                CHECKNEWOBJECT( obj, cx, "object_id_constructor" );
                *rval = OBJECT_TO_JSVAL( obj );
            }

            jsval v = c.toval( oid.str().c_str() );
            verify( JS_SetProperty( cx , obj , "str" , &v  ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16306 );
        }
        return JS_TRUE;
    }

    JSClass object_id_class = {
        "ObjectId" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    // dbpointer

    JSBool dbpointer_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            if ( argc != 2 ) {
                JS_ReportError( cx, "DBPointer takes 2 arguments -- DBPointer(namespace,objectId)" );
                return JS_FALSE;
            }
            if ( ! JSVAL_IS_OID( argv[1] ) ) {
                JS_ReportError( cx , "2nd argument to DBPointer must be objectId" );
                return JS_FALSE;
            }
            Convertor c( cx );
            if ( ! JS_InstanceOf( cx , obj , &dbpointer_class , 0 ) ) {
                obj = JS_NewObject( cx , &dbpointer_class , 0 , 0 );
                CHECKNEWOBJECT( obj, cx, "dbpointer_constructor" );
                *rval = OBJECT_TO_JSVAL( obj );
            }
            verify( JS_SetProperty( cx , obj , "ns" , &(argv[0]) ) );
            verify( JS_SetProperty( cx , obj , "id" , &(argv[1]) ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16307 );
        }
        return JS_TRUE;
    }

    JSClass dbpointer_class = {
        "DBPointer" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSFunctionSpec dbpointer_functions[] = {
        { 0 }
    };


    JSBool dbref_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            if ( argc != 2 ) {
                JS_ReportError( cx , "DBRef takes 2 arguments -- DBRef($ref,$id)" );
                verify( JS_SetPrivate( cx , obj , (void*)(new BSONHolder( BSONObj().getOwned() ) ) ) );
                return JS_FALSE;
            }
            Convertor c( cx );
            if ( ! JS_InstanceOf( cx , obj , &dbref_class , 0 ) ) {
                obj = JS_NewObject( cx , &dbref_class , 0 , 0 );
                CHECKNEWOBJECT( obj, cx, "dbref_constructor" );
                *rval = OBJECT_TO_JSVAL( obj );
            }
            JSObject * o = JS_NewObject( cx , NULL , NULL, NULL );
            CHECKNEWOBJECT( o, cx, "dbref_constructor" );
            verify( JS_SetProperty( cx, o , "$ref" , &argv[ 0 ] ) );
            verify( JS_SetProperty( cx, o , "$id" , &argv[ 1 ] ) );
            BSONObj bo = c.toObject( o );
            verify( JS_SetPrivate( cx , obj , (void*)(new BSONHolder( bo.getOwned() ) ) ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16308 );
        }
        return JS_TRUE;
    }

    JSClass dbref_class = bson_class; // name will be fixed later

    // UUID **************************

#if 0
    JSBool uuid_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        Convertor c( cx );

        if( argc == 0 ) {
#if defined(HAVE_UUID)
            //uuids::uuid
#else
#endif
            JS_ReportError( cx , "UUID needs 1 argument -- UUID(hexstr)" );
            return JS_FALSE;
        }
        else if ( argc == 1 ) {

            string encoded = c.toString( argv[ 0 ] );
            if( encoded.size() != 32 ) {
                JS_ReportError( cx, "expect 32 char hex string to UUID()" );
                return JS_FALSE;
            }

            char buf[16];
            for( int i = 0; i < 16; i++ ) {
                buf[i] = fromHex(encoded.c_str() + i * 2);
            }

zzz

            verify( JS_SetPrivate( cx, obj, new BinDataHolder( buf, 16 ) ) );
            c.setProperty( obj, "len", c.toval( (double)16 ) );
            c.setProperty( obj, "type", c.toval( (double)3 ) );

            return JS_TRUE;
        }
        else {
            JS_ReportError( cx , "UUID needs 1 argument -- UUID(hexstr)" );
            return JS_FALSE;
        }
    }

    JSBool uuid_tostring(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        Convertor c(cx);
        void *holder = JS_GetPrivate( cx, obj );
        verify( holder );
        const char *data = ( ( BinDataHolder* )( holder ) )->c_;
        stringstream ss;
        ss << "UUID(\"" << toHex(data, 16);
        ss << "\")";
        string ret = ss.str();
        return *rval = c.toval( ret.c_str() );
    }

    void uuid_finalize( JSContext * cx , JSObject * obj ) {
        Convertor c(cx);
        void *holder = JS_GetPrivate( cx, obj );
        if ( holder ) {
            delete ( BinDataHolder* )holder;
            verify( JS_SetPrivate( cx , obj , 0 ) );
        }
    }

    JSClass uuid_class = {
        "UUID" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, uuid_finalize,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSFunctionSpec uuid_functions[] = {
        { "toString" , uuid_tostring , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { 0 }
    };

#endif

    // BinData **************************

    JSBool bindata_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            if ( argc != 2 ) {
                JS_ReportError( cx , "BinData takes 2 arguments -- BinData(subtype,data)" );
                return JS_FALSE;
            }
            Convertor c( cx );
            int subtype = static_cast<int>( c.toNumber( argv[ 0 ] ) );
            if ( subtype < 0 || subtype > 255 ) {
                JS_ReportError( cx, "BinData subtype must be between 0 and 255" );
                return JS_FALSE;
            }
            string encoded( c.toString( argv[ 1 ] ) );
            string decoded;
            try {
                decoded = base64::decode( encoded );
            }
            catch(...) {
                JS_ReportError( cx, "BinData could not decode base64 parameter" );
                return JS_FALSE;
            }
            if ( ! JS_InstanceOf( cx , obj , &bindata_class , 0 ) ) {
                obj = JS_NewObject( cx , &bindata_class , 0 , 0 );
                CHECKNEWOBJECT( obj, cx, "bindata_constructor" );
                *rval = OBJECT_TO_JSVAL( obj );
            }
            verify( JS_SetPrivate( cx, obj, new BinDataHolder( decoded.data(), decoded.length() ) ) );
            c.setProperty( obj, "len", c.toval( (double)decoded.length() ) );
            c.setProperty( obj, "type", c.toval( (double)subtype ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16309 );
        }
        return JS_TRUE;
    }

    JSBool bindata_tostring(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            Convertor c(cx);
            int type = static_cast<int>( c.getNumber( obj, "type" ) );
            int len = static_cast<int>(c.getNumber( obj, "len" ) );
            void *holder = JS_GetPrivate( cx, obj );
            verify( holder );
            const char *data = ( ( BinDataHolder* )( holder ) )->c_;
            stringstream ss;
            ss << "BinData(" << type << ",\"";
            base64::encode( ss, data, len );
            ss << "\")";
            string ret = ss.str();
            *rval = c.toval( ret.c_str() );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16310 );
        }
        return JS_TRUE;
    }

    JSBool bindataBase64(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            Convertor c(cx);
            int len = static_cast<int>( c.getNumber( obj, "len" ) );
            void *holder = JS_GetPrivate( cx, obj );
            verify( holder );
            const char *data = ( ( BinDataHolder* )( holder ) )->c_;
            stringstream ss;
            base64::encode( ss, data, len );
            string ret = ss.str();
            *rval = c.toval( ret.c_str() );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16311 );
        }
        return JS_TRUE;
    }

    JSBool bindataAsHex(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            Convertor c(cx);
            int len = static_cast<int>( c.getNumber( obj, "len" ) );
            void *holder = JS_GetPrivate( cx, obj );
            verify( holder );
            const char *data = ( ( BinDataHolder* )( holder ) )->c_;
            stringstream ss;
            ss.setf( ios_base::hex, ios_base::basefield );
            ss.fill( '0' );
            ss.setf( ios_base::right, ios_base::adjustfield );
            for( int i = 0; i < len; i++ ) {
                unsigned v = static_cast<unsigned char>( data[i] );
                ss << setw(2) << v;
            }
            string ret = ss.str();
            *rval = c.toval( ret.c_str() );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16312 );
        }
        return JS_TRUE;
    }

    void bindata_finalize( JSContext * cx , JSObject * obj ) {
        try {
            void *holder = JS_GetPrivate( cx, obj );
            if ( holder ) {
                delete reinterpret_cast<BinDataHolder*>( holder );
                verify( JS_SetPrivate( cx , obj , 0 ) );
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16313 );
        }
    }

    JSClass bindata_class = {
        "BinData" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, bindata_finalize,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSFunctionSpec bindata_functions[] = {
        { "toString" , bindata_tostring , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "hex", bindataAsHex, 0, JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "base64", bindataBase64, 0, JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { 0 }
    };

    // Map

    bool specialMapString( const string& s ) {
        return s == "put" || s == "get" || s == "_get" || s == "values" || s == "_data" || s == "constructor" ;
    }

    JSBool map_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            if ( argc > 0 ) {
                JS_ReportError( cx , "Map takes no arguments" );
                return JS_FALSE;
            }

            JSObject * arrayObj = JS_NewObject( cx , 0 , 0 , 0 );
            CHECKNEWOBJECT( arrayObj, cx, "map_constructor" );

            jsval a = OBJECT_TO_JSVAL( arrayObj );
            JS_SetProperty( cx , obj , "_data" , &a );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16314 );
        }
        return JS_TRUE;
    }

    JSBool map_prop( JSContext *cx, JSObject *obj, jsval idval, jsval *vp ) {
        try {
            Convertor c(cx);
            string str( c.toString( idval ) );
            if ( ! specialMapString( str ) ) {
                log() << "illegal prop access: " << str << endl;
                JS_ReportError( cx , "can't use array access with Map" );
                return JS_FALSE;
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16315 );
        }
        return JS_TRUE;
    }

    JSClass map_class = {
        "Map" , JSCLASS_HAS_PRIVATE ,
        map_prop, JS_PropertyStub, map_prop, map_prop,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSFunctionSpec map_functions[] = {
        { 0 }
    };


    // -----

    JSClass timestamp_class = {
        "Timestamp" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSBool timestamp_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            smuassert( cx,
                       "Timestamp takes 0 or 2 arguments -- Timestamp() or Timestamp(t,i)",
                       argc == 0 || argc == 2 );

            if ( ! JS_InstanceOf( cx , obj , &timestamp_class , 0 ) ) {
                obj = JS_NewObject( cx , &timestamp_class , 0 , 0 );
                CHECKNEWOBJECT( obj, cx, "timestamp_constructor" );
                *rval = OBJECT_TO_JSVAL( obj );
            }

            Convertor c( cx );
            if ( argc == 0 ) {
                c.setProperty( obj, "t", c.toval( 0.0 ) );
                c.setProperty( obj, "i", c.toval( 0.0 ) );
            }
            else {
                c.setProperty( obj, "t", argv[ 0 ] );
                c.setProperty( obj, "i", argv[ 1 ] );
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16316 );
        }
        return JS_TRUE;
    }

    JSClass numberlong_class = {
        "NumberLong" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSBool numberlong_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            smuassert( cx , "NumberLong needs 0 or 1 args" , argc == 0 || argc == 1 );

            if ( ! JS_InstanceOf( cx , obj , &numberlong_class , 0 ) ) {
                obj = JS_NewObject( cx , &numberlong_class , 0 , 0 );
                CHECKNEWOBJECT( obj, cx, "numberlong_constructor" );
                *rval = OBJECT_TO_JSVAL( obj );
            }

            Convertor c( cx );
            if ( argc == 0 ) {
                c.setProperty( obj, "floatApprox", c.toval( 0.0 ) );
            }
            else if ( JSVAL_IS_NUMBER( argv[ 0 ] ) ) {
                c.setProperty( obj, "floatApprox", argv[ 0 ] );
            }
            else {
                string num = c.toString( argv[ 0 ] );
                //PRINT(num);
                const char *numStr = num.c_str();
                long long n;
                try {
                    n = parseLL( numStr );
                    //PRINT(n);
                }
                catch ( const AssertionException & ) {
                    smuassert( cx , "could not convert string to long long" , false );
                }
                c.makeLongObj( n, obj );
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16317 );
        }
        return JS_TRUE;
    }

    JSBool numberlong_valueof(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            Convertor c(cx);
            *rval = c.toval( static_cast<double>( c.toNumberLongUnsafe( obj ) ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16318 );
        }
        return JS_TRUE;
    }

    JSBool numberlong_tonumber(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        return numberlong_valueof( cx, obj, argc, argv, rval );
    }

    JSBool numberlong_tostring(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            Convertor c(cx);
            stringstream ss;
            long long val = c.toNumberLongUnsafe( obj );
            const long long limit = 2LL << 30;

            if ( val <= -limit || limit <= val )
                ss << "NumberLong(\"" << val << "\")";
            else
                ss << "NumberLong(" << val << ")";

            string ret = ss.str();
            *rval = c.toval( ret.c_str() );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16319 );
        }
        return JS_TRUE;
    }

    JSFunctionSpec numberlong_functions[] = {
        { "valueOf" , numberlong_valueof , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "toNumber" , numberlong_tonumber , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "toString" , numberlong_tostring , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { 0 }
    };

    JSClass numberint_class = {
        "NumberInt" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSBool numberint_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            smuassert( cx , "NumberInt needs 0 or 1 args" , argc == 0 || argc == 1 );

            if ( ! JS_InstanceOf( cx , obj , &numberint_class , 0 ) ) {
                obj = JS_NewObject( cx , &numberint_class , 0 , 0 );
                CHECKNEWOBJECT( obj, cx, "numberint_constructor" );
                *rval = OBJECT_TO_JSVAL( obj );
            }

            Convertor c( cx );
            if ( argc == 0 ) {
                c.setProperty( obj, "floatApprox", c.toval( 0.0 ) );
            }
            else if ( JSVAL_IS_NUMBER( argv[ 0 ] ) ) {
                c.setProperty( obj, "floatApprox", argv[ 0 ] );
            }
            else {
                string num = c.toString( argv[ 0 ] );
                //PRINT(num);
                const char *numStr = num.c_str();
                int n;
                try {
                    n = static_cast<int>( parseLL( numStr ) );
                    //PRINT(n);
                }
                catch ( const AssertionException & ) {
                    smuassert( cx , "could not convert string to integer" , false );
                }
                c.makeIntObj( n, obj );
            }
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16320 );
        }
        return JS_TRUE;
    }

    JSBool numberint_valueof(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            Convertor c(cx);
            *rval = c.toval( static_cast<double>( c.toNumberInt( obj ) ) );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16321 );
        }
        return JS_TRUE;
    }

    JSBool numberint_tonumber(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        return numberint_valueof( cx, obj, argc, argv, rval );
    }

    JSBool numberint_tostring(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
        try {
            Convertor c(cx);
            int val = c.toNumberInt( obj );
            string ret = str::stream() << "NumberInt(" << val << ")";
            *rval = c.toval( ret.c_str() );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16322 );
        }
        return JS_TRUE;
    }

    JSFunctionSpec numberint_functions[] = {
        { "valueOf" , numberint_valueof , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "toNumber" , numberint_tonumber , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { "toString" , numberint_tostring , 0 , JSPROP_READONLY | JSPROP_PERMANENT, 0 } ,
        { 0 }
    };

    JSClass minkey_class = {
        "MinKey" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    JSClass maxkey_class = {
        "MaxKey" , JSCLASS_HAS_PRIVATE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, JS_ResolveStub , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    // dbquery

    JSBool dbquery_constructor( JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval ) {
        try {
            smuassert( cx ,  "DDQuery needs at least 4 args" , argc >= 4 );

            Convertor c(cx);
            c.setProperty( obj , "_mongo" , argv[0] );
            c.setProperty( obj , "_db" , argv[1] );
            c.setProperty( obj , "_collection" , argv[2] );
            c.setProperty( obj , "_ns" , argv[3] );

            if ( argc > 4 && JSVAL_IS_OBJECT( argv[4] ) )
                c.setProperty( obj , "_query" , argv[4] );
            else {
                JSObject * temp = JS_NewObject( cx , 0 , 0 , 0 );
                CHECKNEWOBJECT( temp, cx, "dbquery_constructor" );
                c.setProperty( obj , "_query" , OBJECT_TO_JSVAL( temp ) );
            }

            if ( argc > 5 && JSVAL_IS_OBJECT( argv[5] ) )
                c.setProperty( obj , "_fields" , argv[5] );
            else
                c.setProperty( obj , "_fields" , JSVAL_NULL );


            if ( argc > 6 && JSVAL_IS_NUMBER( argv[6] ) )
                c.setProperty( obj , "_limit" , argv[6] );
            else
                c.setProperty( obj , "_limit" , JSVAL_ZERO );

            if ( argc > 7 && JSVAL_IS_NUMBER( argv[7] ) )
                c.setProperty( obj , "_skip" , argv[7] );
            else
                c.setProperty( obj , "_skip" , JSVAL_ZERO );

            if ( argc > 8 && JSVAL_IS_NUMBER( argv[8] ) )
                c.setProperty( obj , "_batchSize" , argv[8] );
            else
                c.setProperty( obj , "_batchSize" , JSVAL_ZERO );

            if ( argc > 9 && JSVAL_IS_NUMBER( argv[9] ) )
                c.setProperty( obj , "_options" , argv[9] );
            else
                c.setProperty( obj , "_options" , JSVAL_ZERO );

            c.setProperty( obj , "_cursor" , JSVAL_NULL );
            c.setProperty( obj , "_numReturned" , JSVAL_ZERO );
            c.setProperty( obj , "_special" , JSVAL_FALSE );
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16323 );
        }
        return JS_TRUE;
    }

    JSBool dbquery_resolve( JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **objp ) {
        try {
            if ( flags & JSRESOLVE_ASSIGNING )
                return JS_TRUE;

            if ( ! JSVAL_IS_NUMBER( id ) )
                return JS_TRUE;

            jsval val = JSVAL_VOID;
            verify( JS_CallFunctionName( cx , obj , "arrayAccess" , 1 , &id , &val ) );
            Convertor c(cx);
            c.setProperty( obj , c.toString( id ).c_str() , val );
            *objp = obj;
        }
        catch ( const AssertionException& e ) {
            if ( ! JS_IsExceptionPending( cx ) ) {
                JS_ReportError( cx, e.what() );
            }
            return JS_FALSE;
        }
        catch ( const std::exception& e ) {
            log() << "unhandled exception: " << e.what() << ", throwing Fatal Assertion" << endl;
            fassertFailed( 16324 );
        }
        return JS_TRUE;
    }

    JSClass dbquery_class = {
        "DBQuery" , JSCLASS_NEW_RESOLVE ,
        JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
        JS_EnumerateStub, (JSResolveOp)(&dbquery_resolve) , JS_ConvertStub, JS_FinalizeStub,
        JSCLASS_NO_OPTIONAL_MEMBERS
    };

    // ---- other stuff ----

    void initMongoJS( SMScope * scope , JSContext * cx , JSObject * global , bool local ) {

        verify( JS_InitClass( cx , global , 0 , &mongo_class , local ? mongo_local_constructor : mongo_external_constructor , 0 , 0 , mongo_functions , 0 , 0 ) );

        verify( JS_InitClass( cx , global , 0 , &object_id_class , object_id_constructor , 0 , 0 , 0 , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &db_class , db_constructor , 2 , 0 , 0 , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &db_collection_class , db_collection_constructor , 4 , 0 , 0 , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &internal_cursor_class , internal_cursor_constructor , 0 , 0 , internal_cursor_functions , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &dbquery_class , dbquery_constructor , 0 , 0 , 0 , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &dbpointer_class , dbpointer_constructor , 0 , 0 , dbpointer_functions , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &bindata_class , bindata_constructor , 0 , 0 , bindata_functions , 0 , 0 ) );
//        verify( JS_InitClass( cx , global , 0 , &uuid_class , uuid_constructor , 0 , 0 , uuid_functions , 0 , 0 ) );

        verify( JS_InitClass( cx , global , 0 , &timestamp_class , timestamp_constructor , 0 , 0 , 0 , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &numberlong_class , numberlong_constructor , 0 , 0 , numberlong_functions , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &numberint_class , numberint_constructor , 0 , 0 , numberint_functions , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &minkey_class , 0 , 0 , 0 , 0 , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &maxkey_class , 0 , 0 , 0 , 0 , 0 , 0 ) );

        verify( JS_InitClass( cx , global , 0 , &map_class , map_constructor , 0 , 0 , map_functions , 0 , 0 ) );

        verify( JS_InitClass( cx , global , 0 , &bson_ro_class , bson_cons , 0 , 0 , bson_functions , 0 , 0 ) );
        verify( JS_InitClass( cx , global , 0 , &bson_class , bson_cons , 0 , 0 , bson_functions , 0 , 0 ) );

        static const char *dbrefName = "DBRef";
        dbref_class.name = dbrefName;
        verify( JS_InitClass( cx , global , 0 , &dbref_class , dbref_constructor , 2 , 0 , bson_functions , 0 , 0 ) );

        scope->execSetup(JSFiles::assert);
        scope->execSetup(JSFiles::types);

        scope->execCoreFiles();
    }

    bool appendSpecialDBObject( Convertor * c , BSONObjBuilder& b , const string& name , jsval val , JSObject * o ) {

        if ( JS_InstanceOf( c->_context , o , &object_id_class , 0 ) ) {
            OID oid;
            oid.init( c->getString( o , "str" ) );
            b.append( name , oid );
            return true;
        }

        if ( JS_InstanceOf( c->_context , o , &minkey_class , 0 ) ) {
            b.appendMinKey( name );
            return true;
        }

        if ( JS_InstanceOf( c->_context , o , &maxkey_class , 0 ) ) {
            b.appendMaxKey( name );
            return true;
        }

        if ( JS_InstanceOf( c->_context , o , &timestamp_class , 0 ) ) {
            b.appendTimestamp( name , (unsigned long long)c->getNumber( o , "t" ) , (unsigned int )c->getNumber( o , "i" ) );
            return true;
        }

        if ( JS_InstanceOf( c->_context , o , &numberlong_class , 0 ) ) {
            b.append( name , c->toNumberLongUnsafe( o ) );
            return true;
        }

        if ( JS_InstanceOf( c->_context , o , &numberint_class , 0 ) ) {
            b.append( name , c->toNumberInt( o ) );
            return true;
        }

        if ( JS_InstanceOf( c->_context , o , &dbpointer_class , 0 ) ) {
            b.appendDBRef( name , c->getString( o , "ns" ) , c->toOID( c->getProperty( o , "id" ) ) );
            return true;
        }

        if ( JS_InstanceOf( c->_context , o , &bindata_class , 0 ) ) {
            void *holder = JS_GetPrivate( c->_context , o );
            const char *data = ( ( BinDataHolder * )( holder ) )->c_;
            b.appendBinData( name ,
                             (int)(c->getNumber( o , "len" )) , (BinDataType)((char)(c->getNumber( o , "type" ) ) ) ,
                             data
                           );
            return true;
        }

#if defined( SM16 ) || defined( MOZJS )
#warning dates do not work in your version of spider monkey
        {
            jsdouble d = js_DateGetMsecSinceEpoch( c->_context , o );
            if ( d ) {
                b.appendDate( name , Date_t(d) );
                return true;
            }
        }
#elif defined( XULRUNNER )
        if ( JS_InstanceOf( c->_context , o, globalSMEngine->_dateClass , 0 ) ) {
            jsdouble d = js_DateGetMsecSinceEpoch( c->_context , o );
            b.appendDate( name , Date_t(d) );
            return true;
        }
#else
        if ( JS_InstanceOf( c->_context , o, &js_DateClass , 0 ) ) {
            jsdouble d = js_DateGetMsecSinceEpoch( c->_context , o );
            long long d2 = (long long)d;
            b.appendDate( name , Date_t((unsigned long long)d2) );
            return true;
        }
#endif


        if ( JS_InstanceOf( c->_context , o , &dbquery_class , 0 ) ||
                JS_InstanceOf( c->_context , o , &mongo_class , 0 ) ||
                JS_InstanceOf( c->_context , o , &db_collection_class , 0 ) ) {
            b.append( name , c->toString( val ) );
            return true;
        }

#if defined( XULRUNNER )
        if ( JS_InstanceOf( c->_context , o , globalSMEngine->_regexClass , 0 ) ) {
            c->appendRegex( b , name , c->toString( val ) );
            return true;
        }
#elif defined( SM18 )
        if ( JS_InstanceOf( c->_context , o , &js_RegExpClass , 0 ) ) {
            c->appendRegex( b , name , c->toString( val ) );
            return true;
        }
#endif

        return false;
    }

    bool isDate( JSContext * cx , JSObject * o ) {
#if defined( SM16 ) || defined( MOZJS ) || defined( XULRUNNER )
        return js_DateGetMsecSinceEpoch( cx , o ) != 0;
#else
        return JS_InstanceOf( cx , o, &js_DateClass, 0 );
#endif
    }

}  // namespace spidermonkey
}  // namespace mongo
