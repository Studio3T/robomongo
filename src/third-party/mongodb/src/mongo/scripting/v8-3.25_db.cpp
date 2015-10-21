// v8_db.cpp

/*    Copyright 2014 MongoDB Inc.
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
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects
 *    for all of the code used other than as permitted herein. If you modify
 *    file(s) with this exception, you may extend this exception to your
 *    version of the file(s), but you are not obligated to do so. If you do not
 *    wish to do so, delete this exception statement from your version. If you
 *    delete this exception statement from all source files in the program,
 *    then also delete it in the license file.
 */

#include "mongo/scripting/v8-3.25_db.h"

#include <iostream>
#include <iomanip>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>

#include "mongo/base/init.h"
#include "mongo/client/sasl_client_authenticate.h"
#include "mongo/client/syncclusterconnection.h"
#include "mongo/db/namespace_string.h"
#include "mongo/s/d_state.h"
#include "mongo/scripting/engine_v8-3.25.h"
#include "mongo/scripting/v8-3.25_utils.h"
#include "mongo/util/assert_util.h"
#include "mongo/util/base64.h"
#include "mongo/util/hex.h"
#include "mongo/util/text.h"

using namespace std;
using boost::scoped_array;
using boost::shared_ptr;

namespace mongo {

namespace {
std::vector<V8FunctionPrototypeManipulatorFn> _mongoPrototypeManipulators;
bool _mongoPrototypeManipulatorsFrozen = false;

MONGO_INITIALIZER(V8MongoPrototypeManipulatorRegistry)(InitializerContext* context) {
    return Status::OK();
}

MONGO_INITIALIZER_WITH_PREREQUISITES(V8MongoPrototypeManipulatorRegistrationDone,
                                     ("V8MongoPrototypeManipulatorRegistry"))
(InitializerContext* context) {
    _mongoPrototypeManipulatorsFrozen = true;
    return Status::OK();
}

}  // namespace

void v8RegisterMongoPrototypeManipulator(const V8FunctionPrototypeManipulatorFn& manipulator) {
    fassert(16467, !_mongoPrototypeManipulatorsFrozen);
    _mongoPrototypeManipulators.push_back(manipulator);
}

static v8::Local<v8::Value> newInstance(v8::Local<v8::Function> f,
                                        const v8::FunctionCallbackInfo<v8::Value>& args) {
    // need to translate arguments into an array
    v8::EscapableHandleScope handle_scope(args.GetIsolate());
    const int argc = args.Length();
    static const int MAX_ARGC = 24;
    uassert(16858, "Too many arguments. Max is 24", argc <= MAX_ARGC);

    // TODO SERVER-8016: properly allocate handles on the stack
    v8::Local<v8::Value> argv[MAX_ARGC];
    for (int i = 0; i < argc; ++i) {
        argv[i] = args[i];
    }
    return handle_scope.Escape(f->NewInstance(argc, argv));
}

v8::Local<v8::FunctionTemplate> getInternalCursorFunctionTemplate(V8Scope* scope) {
    v8::Local<v8::FunctionTemplate> ic = scope->createV8Function(internalCursorCons);
    ic->InstanceTemplate()->SetInternalFieldCount(1);
    v8::Local<v8::ObjectTemplate> icproto = ic->PrototypeTemplate();
    scope->injectV8Method("next", internalCursorNext, icproto);
    scope->injectV8Method("hasNext", internalCursorHasNext, icproto);
    scope->injectV8Method("objsLeftInBatch", internalCursorObjsLeftInBatch, icproto);
    scope->injectV8Method("readOnly", internalCursorReadOnly, icproto);
    return ic;
}

v8::Local<v8::FunctionTemplate> getMongoFunctionTemplate(V8Scope* scope, bool local) {
    v8::Local<v8::FunctionTemplate> mongo;
    if (local)
        mongo = scope->createV8Function(mongoConsLocal);
    else
        mongo = scope->createV8Function(mongoConsExternal);
    mongo->InstanceTemplate()->SetInternalFieldCount(1);
    v8::Local<v8::ObjectTemplate> proto = mongo->PrototypeTemplate();
    scope->injectV8Method("find", mongoFind, proto);
    scope->injectV8Method("insert", mongoInsert, proto);
    scope->injectV8Method("remove", mongoRemove, proto);
    scope->injectV8Method("update", mongoUpdate, proto);
    scope->injectV8Method("auth", mongoAuth, proto);
    scope->injectV8Method("logout", mongoLogout, proto);
    scope->injectV8Method("cursorFromId", mongoCursorFromId, proto);

    fassert(16468, _mongoPrototypeManipulatorsFrozen);
    for (size_t i = 0; i < _mongoPrototypeManipulators.size(); ++i)
        _mongoPrototypeManipulators[i](scope, mongo);

    return mongo;
}


v8::Local<v8::Value> mongoConsExternal(V8Scope* scope,
                                       const v8::FunctionCallbackInfo<v8::Value>& args) {
    string host = "127.0.0.1";
    if (args.Length() > 0 && args[0]->IsString()) {
        v8::String::Utf8Value utf(args[0]);
        host = string(*utf);
    }

    // only allow function template to be used by a constructor
    uassert(16859, "Mongo function is only usable as a constructor", args.IsConstructCall());
    verify(scope->MongoFT()->HasInstance(args.This()));

    string errmsg;
    ConnectionString cs = ConnectionString::parse(host, errmsg);
    if (!cs.isValid()) {
        return v8AssertionException(errmsg);
    }

    DBClientBase* conn;
    conn = cs.connect(errmsg);
    if (!conn) {
        return v8AssertionException(errmsg);
    }

    v8::Local<v8::External> connHandle =
        scope->dbClientBaseTracker.track(scope->getIsolate(), args.This(), conn);

    ScriptEngine::runConnectCallback(*conn);

    args.This()->SetInternalField(0, connHandle);
    args.This()->ForceSet(scope->v8StringData("slaveOk"),
                          v8::Boolean::New(scope->getIsolate(), false));
    args.This()->ForceSet(scope->v8StringData("host"), scope->v8StringData(host));

    return v8::Undefined(scope->getIsolate());
}

v8::Local<v8::Value> mongoConsLocal(V8Scope* scope,
                                    const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 0, "local Mongo constructor takes no args")

        // only allow function template to be used by a constructor
        uassert(16860, "Mongo function is only usable as a constructor", args.IsConstructCall());
    verify(scope->MongoFT()->HasInstance(args.This()));

    DBClientBase* conn = createDirectClient(scope->getOpContext());
    v8::Local<v8::External> connHandle =
        scope->dbClientBaseTracker.track(scope->getIsolate(), args.This(), conn);

    args.This()->SetInternalField(0, connHandle);
    args.This()->ForceSet(scope->v8StringData("slaveOk"),
                          v8::Boolean::New(scope->getIsolate(), false));
    args.This()->ForceSet(scope->v8StringData("host"), scope->v8StringData("EMBEDDED"));

    return v8::Undefined(scope->getIsolate());
}

boost::shared_ptr<mongo::DBClientBase> getConnection(
    V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    verify(scope->MongoFT()->HasInstance(args.This()));
    verify(args.This()->InternalFieldCount() == 1);
    v8::Local<v8::External> c = v8::Local<v8::External>::Cast(args.This()->GetInternalField(0));
    boost::shared_ptr<DBClientBase>* conn =
        static_cast<boost::shared_ptr<DBClientBase>*>(c->Value());
    massert(16667, "Unable to get db client connection", conn && conn->get());
    return *conn;
}

/**
 * JavaScript binding for Mongo.prototype.find(namespace, query, fields, limit, skip)
 */
v8::Local<v8::Value> mongoFind(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 7, "find needs 7 args")
        argumentCheck(args[1]->IsObject(), "needs to be an object")
            boost::shared_ptr<DBClientBase> conn = getConnection(scope, args);
    const string ns = toSTLString(args[0]);
    BSONObj fields;
    BSONObj q = scope->v8ToMongo(args[1]->ToObject());
    bool haveFields = args[2]->IsObject() && args[2]->ToObject()->GetPropertyNames()->Length() > 0;
    if (haveFields)
        fields = scope->v8ToMongo(args[2]->ToObject());

    boost::shared_ptr<mongo::DBClientCursor> cursor;
    int nToReturn = args[3]->Int32Value();
    int nToSkip = args[4]->Int32Value();
    int batchSize = args[5]->Int32Value();
    int options = args[6]->Int32Value();
    cursor =
        conn->query(ns, q, nToReturn, nToSkip, haveFields ? &fields : NULL, options, batchSize);
    if (!cursor.get()) {
        return v8AssertionException("error doing query: failed");
    }

    v8::Local<v8::Function> cons = scope->InternalCursorFT()->GetFunction();
    v8::Local<v8::Object> c = cons->NewInstance();
    c->SetInternalField(0, v8::External::New(scope->getIsolate(), cursor.get()));
    scope->dbConnectionAndCursor.track(
        scope->getIsolate(), c, new V8Scope::DBConnectionAndCursor(conn, cursor));
    return c;
}

v8::Local<v8::Value> mongoCursorFromId(V8Scope* scope,
                                       const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 2 || args.Length() == 3, "cursorFromId needs 2 or 3 args")
        argumentCheck(scope->NumberLongFT()->HasInstance(args[1]), "2nd arg must be a NumberLong")
            argumentCheck(args[2]->IsUndefined() || args[2]->IsNumber(),
                          "3rd arg must be a js Number")

                boost::shared_ptr<DBClientBase> conn = getConnection(scope, args);
    const string ns = toSTLString(args[0]);
    long long cursorId = numberLongVal(scope, args[1]->ToObject());

    boost::shared_ptr<mongo::DBClientCursor> cursor(
        new DBClientCursor(conn.get(), ns, cursorId, 0, 0));

    if (!args[2]->IsUndefined())
        cursor->setBatchSize(args[2]->Int32Value());

    v8::Local<v8::Function> cons = scope->InternalCursorFT()->GetFunction();
    v8::Local<v8::Object> c = cons->NewInstance();
    c->SetInternalField(0, v8::External::New(scope->getIsolate(), cursor.get()));
    scope->dbConnectionAndCursor.track(
        scope->getIsolate(), c, new V8Scope::DBConnectionAndCursor(conn, cursor));
    return c;
}

v8::Local<v8::Value> mongoInsert(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 3, "insert needs 3 args")
        argumentCheck(args[1]->IsObject(), "attempted to insert a non-object")

            verify(scope->MongoFT()->HasInstance(args.This()));

    if (args.This()->Get(scope->v8StringData("readOnly"))->BooleanValue()) {
        return v8AssertionException("js db in read only mode");
    }

    boost::shared_ptr<DBClientBase> conn = getConnection(scope, args);
    const string ns = toSTLString(args[0]);

    v8::Local<v8::Integer> flags = args[2]->ToInteger();

    if (args[1]->IsArray()) {
        v8::Local<v8::Array> arr = v8::Local<v8::Array>::Cast(args[1]);
        vector<BSONObj> bos;
        uint32_t len = arr->Length();
        argumentCheck(len > 0, "attempted to insert an empty array")

            for (uint32_t i = 0; i < len; i++) {
            v8::Local<v8::Object> el = arr->CloneElementAt(i);
            argumentCheck(!el.IsEmpty(), "attempted to insert an array of non-object types")

                // Set ID on the element if necessary
                if (!el->Has(scope->v8StringData("_id"))) {
                v8::Local<v8::Value> argv[1];
                el->ForceSet(scope->v8StringData("_id"),
                             scope->ObjectIdFT()->GetFunction()->NewInstance(0, argv));
            }
            bos.push_back(scope->v8ToMongo(el));
        }
        conn->insert(ns, bos, flags->Int32Value());
    } else {
        v8::Local<v8::Object> in = args[1]->ToObject();
        if (!in->Has(scope->v8StringData("_id"))) {
            v8::Local<v8::Value> argv[1];
            in->ForceSet(scope->v8StringData("_id"),
                         scope->ObjectIdFT()->GetFunction()->NewInstance(0, argv));
        }
        BSONObj o = scope->v8ToMongo(in);
        conn->insert(ns, o);
    }
    return v8::Undefined(scope->getIsolate());
}

v8::Local<v8::Value> mongoRemove(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 2 || args.Length() == 3, "remove needs 2 or 3 args")
        argumentCheck(args[1]->IsObject(), "attempted to remove a non-object")

            verify(scope->MongoFT()->HasInstance(args.This()));

    if (args.This()->Get(scope->v8StringData("readOnly"))->BooleanValue()) {
        return v8AssertionException("js db in read only mode");
    }

    boost::shared_ptr<DBClientBase> conn = getConnection(scope, args);
    const string ns = toSTLString(args[0]);

    v8::Local<v8::Object> in = args[1]->ToObject();
    BSONObj o = scope->v8ToMongo(in);

    bool justOne = false;
    if (args.Length() > 2) {
        justOne = args[2]->BooleanValue();
    }

    conn->remove(ns, o, justOne);
    return v8::Undefined(scope->getIsolate());
}

v8::Local<v8::Value> mongoUpdate(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() >= 3, "update needs at least 3 args")
        argumentCheck(args[1]->IsObject(), "1st param to update has to be an object")
            argumentCheck(args[2]->IsObject(), "2nd param to update has to be an object")

                verify(scope->MongoFT()->HasInstance(args.This()));

    if (args.This()->Get(scope->v8StringData("readOnly"))->BooleanValue()) {
        return v8AssertionException("js db in read only mode");
    }

    boost::shared_ptr<DBClientBase> conn = getConnection(scope, args);
    const string ns = toSTLString(args[0]);

    v8::Local<v8::Object> q = args[1]->ToObject();
    v8::Local<v8::Object> o = args[2]->ToObject();

    bool upsert = args.Length() > 3 && args[3]->IsBoolean() && args[3]->ToBoolean()->Value();
    bool multi = args.Length() > 4 && args[4]->IsBoolean() && args[4]->ToBoolean()->Value();

    BSONObj q1 = scope->v8ToMongo(q);
    BSONObj o1 = scope->v8ToMongo(o);
    conn->update(ns, q1, o1, upsert, multi);
    return v8::Undefined(scope->getIsolate());
}

v8::Local<v8::Value> mongoAuth(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    boost::shared_ptr<DBClientBase> conn = getConnection(scope, args);
    if (NULL == conn)
        return v8AssertionException("no connection");

    BSONObj params;
    switch (args.Length()) {
        case 1:
            params = scope->v8ToMongo(args[0]->ToObject());
            break;
        case 3:
            params = BSON(saslCommandMechanismFieldName
                          << "MONGODB-CR" << saslCommandUserDBFieldName << toSTLString(args[0])
                          << saslCommandUserFieldName << toSTLString(args[1])
                          << saslCommandPasswordFieldName << toSTLString(args[2]));
            break;
        default:
            return v8AssertionException("mongoAuth takes 1 object or 3 string arguments");
    }
    try {
        conn->auth(params);
    } catch (const DBException& ex) {
        return v8AssertionException(ex.toString());
    }
    return v8::Boolean::New(scope->getIsolate(), true);
}

v8::Local<v8::Value> mongoLogout(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 1, "logout needs 1 arg") boost::shared_ptr<DBClientBase> conn =
        getConnection(scope, args);
    const string db = toSTLString(args[0]);
    BSONObj ret;
    conn->logout(db, ret);
    return scope->mongoToLZV8(ret, false);
}

/**
 * get cursor from v8 argument
 */
mongo::DBClientCursor* getCursor(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    verify(scope->InternalCursorFT()->HasInstance(args.This()));
    verify(args.This()->InternalFieldCount() == 1);
    v8::Local<v8::External> c = v8::Local<v8::External>::Cast(args.This()->GetInternalField(0));
    mongo::DBClientCursor* cursor = static_cast<mongo::DBClientCursor*>(c->Value());
    return cursor;
}

v8::Local<v8::Value> internalCursorCons(V8Scope* scope,
                                        const v8::FunctionCallbackInfo<v8::Value>& args) {
    return v8::Undefined(scope->getIsolate());
}

/**
 * cursor.next()
 */
v8::Local<v8::Value> internalCursorNext(V8Scope* scope,
                                        const v8::FunctionCallbackInfo<v8::Value>& args) {
    mongo::DBClientCursor* cursor = getCursor(scope, args);
    if (!cursor)
        return v8::Undefined(scope->getIsolate());
    BSONObj o = cursor->next();
    bool ro = false;
    if (args.This()->Has(v8::String::NewFromUtf8(scope->getIsolate(), "_ro")))
        ro = args.This()->Get(v8::String::NewFromUtf8(scope->getIsolate(), "_ro"))->BooleanValue();
    return scope->mongoToLZV8(o, ro);
}

/**
 * cursor.hasNext()
 */
v8::Local<v8::Value> internalCursorHasNext(V8Scope* scope,
                                           const v8::FunctionCallbackInfo<v8::Value>& args) {
    mongo::DBClientCursor* cursor = getCursor(scope, args);
    if (!cursor)
        return v8::Boolean::New(scope->getIsolate(), false);
    return v8::Boolean::New(scope->getIsolate(), cursor->more());
}

/**
 * cursor.objsLeftInBatch()
 */
v8::Local<v8::Value> internalCursorObjsLeftInBatch(
    V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    mongo::DBClientCursor* cursor = getCursor(scope, args);
    if (!cursor)
        return v8::Number::New(scope->getIsolate(), 0.0);
    return v8::Number::New(scope->getIsolate(), static_cast<double>(cursor->objsLeftInBatch()));
}

/**
 * cursor.readOnly()
 */
v8::Local<v8::Value> internalCursorReadOnly(V8Scope* scope,
                                            const v8::FunctionCallbackInfo<v8::Value>& args) {
    verify(scope->InternalCursorFT()->HasInstance(args.This()));

    v8::Local<v8::Object> cursor = args.This();
    cursor->ForceSet(v8::String::NewFromUtf8(scope->getIsolate(), "_ro"),
                     v8::Boolean::New(scope->getIsolate(), true));
    return cursor;
}

v8::Local<v8::Value> dbInit(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->DBFT()->GetFunction();
        return newInstance(f, args);
    }

    verify(scope->DBFT()->HasInstance(args.This()));

    argumentCheck(args.Length() == 2, "db constructor requires 2 arguments")

        args.This()->ForceSet(scope->v8StringData("_mongo"), args[0]);
    args.This()->ForceSet(scope->v8StringData("_name"), args[1]);

    for (int i = 0; i < args.Length(); i++) {
        argumentCheck(!args[i]->IsUndefined(), "db initializer called with undefined argument")
    }

    string dbName = toSTLString(args[1]);
    if (!NamespaceString::validDBName(dbName)) {
        return v8AssertionException(str::stream() << "[" << dbName
                                                  << "] is not a valid database name");
    }
    return v8::Undefined(scope->getIsolate());
}

v8::Local<v8::Value> collectionInit(V8Scope* scope,
                                    const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->DBCollectionFT()->GetFunction();
        return newInstance(f, args);
    }

    verify(scope->DBCollectionFT()->HasInstance(args.This()));

    argumentCheck(args.Length() == 4, "collection constructor requires 4 arguments")

        for (int i = 0; i < args.Length(); i++) {
        argumentCheck(!args[i]->IsUndefined(),
                      "collection constructor called with undefined argument")
    }

    args.This()->ForceSet(scope->v8StringData("_mongo"), args[0]);
    args.This()->ForceSet(scope->v8StringData("_db"), args[1]);
    args.This()->ForceSet(scope->v8StringData("_shortName"), args[2]);
    args.This()->ForceSet(v8::String::NewFromUtf8(scope->getIsolate(), "_fullName"), args[3]);

    if (haveLocalShardingInfo(toSTLString(args[3]))) {
        return v8AssertionException("can't use sharded collection from db.eval");
    }

    return v8::Undefined(scope->getIsolate());
}

v8::Local<v8::Value> dbQueryInit(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->DBQueryFT()->GetFunction();
        return newInstance(f, args);
    }

    verify(scope->DBQueryFT()->HasInstance(args.This()));

    argumentCheck(args.Length() >= 4, "dbQuery constructor requires at least 4 arguments")

        v8::Local<v8::Object> t = args.This();
    t->ForceSet(scope->v8StringData("_mongo"), args[0]);
    t->ForceSet(scope->v8StringData("_db"), args[1]);
    t->ForceSet(scope->v8StringData("_collection"), args[2]);
    t->ForceSet(scope->v8StringData("_ns"), args[3]);

    if (args.Length() > 4 && args[4]->IsObject())
        t->ForceSet(scope->v8StringData("_query"), args[4]);
    else
        t->ForceSet(scope->v8StringData("_query"), v8::Object::New(scope->getIsolate()));

    if (args.Length() > 5 && args[5]->IsObject())
        t->ForceSet(scope->v8StringData("_fields"), args[5]);
    else
        t->ForceSet(scope->v8StringData("_fields"), v8::Null(scope->getIsolate()));

    if (args.Length() > 6 && args[6]->IsNumber())
        t->ForceSet(scope->v8StringData("_limit"), args[6]);
    else
        t->ForceSet(scope->v8StringData("_limit"), v8::Number::New(scope->getIsolate(), 0));

    if (args.Length() > 7 && args[7]->IsNumber())
        t->ForceSet(scope->v8StringData("_skip"), args[7]);
    else
        t->ForceSet(scope->v8StringData("_skip"), v8::Number::New(scope->getIsolate(), 0));

    if (args.Length() > 8 && args[8]->IsNumber())
        t->ForceSet(scope->v8StringData("_batchSize"), args[8]);
    else
        t->ForceSet(scope->v8StringData("_batchSize"), v8::Number::New(scope->getIsolate(), 0));

    if (args.Length() > 9 && args[9]->IsNumber())
        t->ForceSet(scope->v8StringData("_options"), args[9]);
    else
        t->ForceSet(scope->v8StringData("_options"), v8::Number::New(scope->getIsolate(), 0));

    t->ForceSet(scope->v8StringData("_cursor"), v8::Null(scope->getIsolate()));
    t->ForceSet(scope->v8StringData("_numReturned"), v8::Number::New(scope->getIsolate(), 0));
    t->ForceSet(scope->v8StringData("_special"), v8::Boolean::New(scope->getIsolate(), false));

    return v8::Undefined(scope->getIsolate());
}

void collectionSetter(v8::Local<v8::String> name,
                      v8::Local<v8::Value> value,
                      const v8::PropertyCallbackInfo<v8::Value>& info) {
    v8::Local<v8::Value> val;
    v8::ReturnValue<v8::Value> result = info.GetReturnValue();
    result.Set(val);
    try {
        V8Scope* scope = getScope(info.GetIsolate());

        // Both DB and Collection objects use this setter
        verify(scope->DBCollectionFT()->HasInstance(info.This()) ||
               scope->DBFT()->HasInstance(info.This()));

        // a collection name cannot be overwritten by a variable
        string sname = toSTLString(name);
        if (sname.length() == 0 || sname[0] == '_') {
            // if starts with '_' we allow overwrite
            return;
        }
        // dont set
        result.Set(value);
    } catch (const DBException& dbEx) {
        result.Set(v8AssertionException(dbEx.toString()));
    } catch (...) {
        result.Set(v8AssertionException("unknown error in collationSetter"));
    }
}

void collectionGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
    v8::Local<v8::Value> val;
    v8::ReturnValue<v8::Value> result = info.GetReturnValue();
    result.Set(val);
    try {
        V8Scope* scope = getScope(info.GetIsolate());

        // Both DB and Collection objects use this getter
        verify(scope->DBCollectionFT()->HasInstance(info.This()) ||
               scope->DBFT()->HasInstance(info.This()));

        v8::TryCatch tryCatch;

        // first look in prototype, may be a function
        v8::Local<v8::Value> real = info.This()->GetPrototype()->ToObject()->Get(name);
        if (!real->IsUndefined()) {
            result.Set(real);
            return;
        }

        // 2nd look into real values, may be cached collection object
        string sname = toSTLString(name);
        if (info.This()->HasRealNamedProperty(name)) {
            v8::Local<v8::Value> prop = info.This()->GetRealNamedProperty(name);
            if (prop->IsObject() &&
                prop->ToObject()->HasRealNamedProperty(
                    v8::String::NewFromUtf8(scope->getIsolate(), "_fullName"))) {
                // need to check every time that the collection did not get sharded
                if (haveLocalShardingInfo(toSTLString(prop->ToObject()->GetRealNamedProperty(
                        v8::String::NewFromUtf8(scope->getIsolate(), "_fullName"))))) {
                    result.Set(v8AssertionException("can't use sharded collection from db.eval"));
                    return;
                }
            }
            result.Set(prop);
            return;
        } else if (sname.length() == 0 || sname[0] == '_') {
            // if starts with '_' we dont return collection, one must use getCollection()
            return;
        }

        // no hit, create new collection
        v8::Local<v8::Value> getCollection = info.This()->GetPrototype()->ToObject()->Get(
            v8::String::NewFromUtf8(scope->getIsolate(), "getCollection"));
        if (!getCollection->IsFunction()) {
            result.Set(v8AssertionException("getCollection is not a function"));
            return;
        }

        v8::Local<v8::Function> f = getCollection.As<v8::Function>();
        v8::Local<v8::Value> argv[1];
        argv[0] = name;
        v8::Local<v8::Value> coll = f->Call(info.This(), 1, argv);
        if (coll.IsEmpty()) {
            result.Set(tryCatch.ReThrow());
            return;
        }

        uassert(16861,
                "getCollection returned something other than a collection",
                scope->DBCollectionFT()->HasInstance(coll));

        // cache collection for reuse, don't enumerate
        info.This()->ForceSet(name, coll, v8::DontEnum);
        result.Set(coll);
    } catch (const DBException& dbEx) {
        result.Set(v8AssertionException(dbEx.toString()));
    } catch (...) {
        result.Set(v8AssertionException("unknown error in collectionGetter"));
    }
}

void dbQueryIndexAccess(unsigned int index, const v8::PropertyCallbackInfo<v8::Value>& info) {
    v8::Local<v8::Value> val;
    v8::ReturnValue<v8::Value> result = info.GetReturnValue();
    result.Set(val);
    try {
        V8Scope* scope = getScope(info.GetIsolate());
        verify(scope->DBQueryFT()->HasInstance(info.This()));

        v8::Local<v8::Value> arrayAccess = info.This()->GetPrototype()->ToObject()->Get(
            v8::String::NewFromUtf8(scope->getIsolate(), "arrayAccess"));
        massert(16660, "arrayAccess is not a function", arrayAccess->IsFunction());

        v8::Local<v8::Function> f = arrayAccess.As<v8::Function>();
        v8::Local<v8::Value> argv[1];
        argv[0] = v8::Number::New(scope->getIsolate(), index);

        result.Set(f->Call(info.This(), 1, argv));
    } catch (const DBException& dbEx) {
        result.Set(v8AssertionException(dbEx.toString()));
    } catch (...) {
        result.Set(v8AssertionException("unknown error in dbQueryIndexAccess"));
    }
}

v8::Local<v8::Value> objectIdInit(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->ObjectIdFT()->GetFunction();
        return newInstance(f, args);
    }

    v8::Local<v8::Object> it = args.This();
    verify(scope->ObjectIdFT()->HasInstance(it));

    OID oid;
    if (args.Length() == 0) {
        oid.init();
    } else {
        string s = toSTLString(args[0]);
        try {
            Scope::validateObjectIdString(s);
        } catch (const MsgAssertionException& m) {
            return v8AssertionException(m.toString());
        }
        oid.init(s);
    }

    it->ForceSet(scope->v8StringData("str"),
                 v8::String::NewFromUtf8(scope->getIsolate(), oid.toString().c_str()));
    return it;
}

v8::Local<v8::Value> dbRefInit(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->DBRefFT()->GetFunction();
        return newInstance(f, args);
    }

    v8::Local<v8::Object> it = args.This();
    verify(scope->DBRefFT()->HasInstance(it));

    argumentCheck(args.Length() >= 2 && args.Length() <= 3, "DBRef needs 2 or 3 arguments")
        argumentCheck(args[0]->IsString(), "DBRef 1st parameter must be a string")
            it->ForceSet(scope->v8StringData("$ref"), args[0]);
    it->ForceSet(scope->v8StringData("$id"), args[1]);

    if (args.Length() == 3) {
        argumentCheck(args[2]->IsString(), "DBRef 3rd parameter must be a string")
            it->ForceSet(scope->v8StringData("$db"), args[2]);
    }

    return it;
}

v8::Local<v8::Value> dbPointerInit(V8Scope* scope,
                                   const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->DBPointerFT()->GetFunction();
        return newInstance(f, args);
    }

    v8::Local<v8::Object> it = args.This();
    verify(scope->DBPointerFT()->HasInstance(it));

    argumentCheck(args.Length() == 2, "DBPointer needs 2 arguments")
        argumentCheck(args[0]->IsString(), "DBPointer 1st parameter must be a string")
            argumentCheck(scope->ObjectIdFT()->HasInstance(args[1]),
                          "DBPointer 2nd parameter must be an ObjectId")

                it->ForceSet(scope->v8StringData("ns"), args[0]);
    it->ForceSet(scope->v8StringData("id"), args[1]);
    return it;
}

v8::Local<v8::Value> dbTimestampInit(V8Scope* scope,
                                     const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->TimestampFT()->GetFunction();
        return newInstance(f, args);
    }

    v8::Local<v8::Object> it = args.This();
    verify(scope->TimestampFT()->HasInstance(it));

    if (args.Length() == 0) {
        it->ForceSet(scope->v8StringData("t"), v8::Number::New(scope->getIsolate(), 0));
        it->ForceSet(scope->v8StringData("i"), v8::Number::New(scope->getIsolate(), 0));
    } else if (args.Length() == 2) {
        if (!args[0]->IsNumber()) {
            return v8AssertionException("Timestamp time must be a number");
        }
        if (!args[1]->IsNumber()) {
            return v8AssertionException("Timestamp increment must be a number");
        }
        int64_t t = args[0]->IntegerValue();
        int64_t largestVal = int64_t(OpTime::max().getSecs());
        if (t > largestVal)
            return v8AssertionException(str::stream() << "The first argument must be in seconds; "
                                                      << t << " is too large (max " << largestVal
                                                      << ")");
        it->ForceSet(scope->v8StringData("t"), args[0]);
        it->ForceSet(scope->v8StringData("i"), args[1]);
    } else {
        return v8AssertionException("Timestamp needs 0 or 2 arguments");
    }

    return it;
}

v8::Local<v8::Value> binDataInit(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->BinDataFT()->GetFunction();
        return newInstance(f, args);
    }

    v8::Local<v8::Object> it = args.This();
    verify(scope->BinDataFT()->HasInstance(it));

    argumentCheck(args.Length() == 2, "BinData takes 2 arguments -- BinData(subtype,data)");

    // 2 args: type, base64 string
    v8::Local<v8::Value> type = args[0];
    if (!type->IsNumber() || type->Int32Value() < 0 || type->Int32Value() > 255) {
        return v8AssertionException(
            "BinData subtype must be a Number between 0 and 255 inclusive)");
    }
    v8::String::Utf8Value utf(args[1]);
    // uassert if invalid base64 string
    string tmpBase64 = base64::decode(*utf);
    // length property stores the decoded length
    it->ForceSet(scope->v8StringData("len"),
                 v8::Number::New(scope->getIsolate(), tmpBase64.length()),
                 v8::ReadOnly);
    it->ForceSet(scope->v8StringData("type"), type, v8::ReadOnly);
    it->SetInternalField(0, args[1]);

    return it;
}

v8::Local<v8::Value> binDataToString(V8Scope* scope,
                                     const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> it = args.This();
    verify(scope->BinDataFT()->HasInstance(it));
    int type = it->Get(v8::String::NewFromUtf8(scope->getIsolate(), "type"))->Int32Value();

    stringstream ss;
    verify(it->InternalFieldCount() == 1);
    ss << "BinData(" << type << ",\"" << toSTLString(it->GetInternalField(0)) << "\")";
    return v8::String::NewFromUtf8(scope->getIsolate(), ss.str().c_str());
}

v8::Local<v8::Value> binDataToBase64(V8Scope* scope,
                                     const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> it = args.This();
    verify(scope->BinDataFT()->HasInstance(it));
    verify(it->InternalFieldCount() == 1);
    return it->GetInternalField(0);
}

v8::Local<v8::Value> binDataToHex(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> it = args.This();
    verify(scope->BinDataFT()->HasInstance(it));
    verify(it->InternalFieldCount() == 1);
    string data = base64::decode(toSTLString(it->GetInternalField(0)));
    stringstream ss;
    ss.setf(ios_base::hex, ios_base::basefield);
    ss.fill('0');
    ss.setf(ios_base::right, ios_base::adjustfield);
    for (std::string::iterator it = data.begin(); it != data.end(); ++it) {
        unsigned v = (unsigned char)*it;
        ss << setw(2) << v;
    }
    return v8::String::NewFromUtf8(scope->getIsolate(), ss.str().c_str());
}

static v8::Local<v8::Value> hexToBinData(V8Scope* scope, int type, string hexstr) {
    // SERVER-9686: This function does not correctly check to make sure hexstr is actually made
    // up of valid hex digits, and fails in the hex utility functions

    int len = hexstr.length() / 2;
    scoped_array<char> data(new char[len]);
    const char* src = hexstr.c_str();
    for (int i = 0; i < len; i++) {
        data[i] = fromHex(src + i * 2);
    }

    string encoded = base64::encode(data.get(), len);
    v8::Local<v8::Value> argv[2];
    argv[0] = v8::Number::New(scope->getIsolate(), type);
    argv[1] = v8::String::NewFromUtf8(scope->getIsolate(), encoded.c_str());
    return scope->BinDataFT()->GetFunction()->NewInstance(2, argv);
}

v8::Local<v8::Value> uuidInit(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 1, "UUID needs 1 argument") v8::String::Utf8Value utf(args[0]);
    argumentCheck(utf.length() == 32,
                  "UUID string must have 32 characters") return hexToBinData(scope, bdtUUID, *utf);
}

v8::Local<v8::Value> md5Init(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 1, "MD5 needs 1 argument") v8::String::Utf8Value utf(args[0]);
    argumentCheck(utf.length() == 32,
                  "MD5 string must have 32 characters") return hexToBinData(scope, MD5Type, *utf);
}

v8::Local<v8::Value> hexDataInit(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 2, "HexData needs 2 arguments") v8::Local<v8::Value> type =
        args[0];
    if (!type->IsNumber() || type->Int32Value() < 0 || type->Int32Value() > 255) {
        return v8AssertionException("HexData subtype must be a Number between 0 and 255 inclusive");
    }
    v8::String::Utf8Value utf(args[1]);
    return hexToBinData(scope, type->Int32Value(), *utf);
}

v8::Local<v8::Value> numberLongInit(V8Scope* scope,
                                    const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->NumberLongFT()->GetFunction();
        return newInstance(f, args);
    }

    argumentCheck(args.Length() == 0 || args.Length() == 1 || args.Length() == 3,
                  "NumberLong needs 0, 1 or 3 arguments")

        v8::Local<v8::Object> it = args.This();
    verify(scope->NumberLongFT()->HasInstance(it));

    if (args.Length() == 0) {
        it->ForceSet(scope->v8StringData("floatApprox"), v8::Number::New(scope->getIsolate(), 0));
    } else if (args.Length() == 1) {
        if (args[0]->IsNumber()) {
            it->ForceSet(scope->v8StringData("floatApprox"), args[0]);
        } else {
            v8::String::Utf8Value data(args[0]);
            string num = *data;
            const char* numStr = num.c_str();
            long long n;
            try {
                n = parseLL(numStr);
            } catch (const AssertionException&) {
                return v8AssertionException(string("could not convert \"") + num +
                                            "\" to NumberLong");
            }
            unsigned long long val = n;
            // values above 2^53 are not accurately represented in JS
            if ((long long)val == (long long)(double)(long long)(val) &&
                val < 9007199254740992ULL) {
                it->ForceSet(scope->v8StringData("floatApprox"),
                             v8::Number::New(scope->getIsolate(), (double)(long long)(val)));
            } else {
                it->ForceSet(scope->v8StringData("floatApprox"),
                             v8::Number::New(scope->getIsolate(), (double)(long long)(val)));
                it->ForceSet(scope->v8StringData("top"),
                             v8::Integer::New(scope->getIsolate(), val >> 32));
                it->ForceSet(scope->v8StringData("bottom"),
                             v8::Integer::New(scope->getIsolate(),
                                              (unsigned long)(val & 0x00000000ffffffff)));
            }
        }
    } else {
        it->ForceSet(scope->v8StringData("floatApprox"), args[0]->ToNumber());
        it->ForceSet(scope->v8StringData("top"), args[1]->ToUint32());
        it->ForceSet(scope->v8StringData("bottom"), args[2]->ToUint32());
    }
    return it;
}

long long numberLongVal(V8Scope* scope, const v8::Local<v8::Object>& it) {
    verify(scope->NumberLongFT()->HasInstance(it));
    if (!it->Has(v8::String::NewFromUtf8(scope->getIsolate(), "top")))
        return (long long)(it->Get(v8::String::NewFromUtf8(scope->getIsolate(), "floatApprox"))
                               ->NumberValue());
    return (long long)((unsigned long long)(it->Get(v8::String::NewFromUtf8(scope->getIsolate(),
                                                                            "top"))
                                                ->ToInt32()
                                                ->Value())
                       << 32) +
        (unsigned)(it->Get(v8::String::NewFromUtf8(scope->getIsolate(), "bottom"))
                       ->ToInt32()
                       ->Value());
}

v8::Local<v8::Value> numberLongValueOf(V8Scope* scope,
                                       const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> it = args.This();
    long long val = numberLongVal(scope, it);
    return v8::Number::New(scope->getIsolate(), double(val));
}

v8::Local<v8::Value> numberLongToNumber(V8Scope* scope,
                                        const v8::FunctionCallbackInfo<v8::Value>& args) {
    return numberLongValueOf(scope, args);
}

v8::Local<v8::Value> numberLongToString(V8Scope* scope,
                                        const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> it = args.This();

    stringstream ss;
    long long val = numberLongVal(scope, it);
    const long long limit = 2LL << 30;

    if (val <= -limit || limit <= val)
        ss << "NumberLong(\"" << val << "\")";
    else
        ss << "NumberLong(" << val << ")";

    string ret = ss.str();
    return v8::String::NewFromUtf8(scope->getIsolate(), ret.c_str());
}

v8::Local<v8::Value> numberIntInit(V8Scope* scope,
                                   const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        v8::Local<v8::Function> f = scope->NumberIntFT()->GetFunction();
        return newInstance(f, args);
    }

    v8::Local<v8::Object> it = args.This();
    verify(scope->NumberIntFT()->HasInstance(it));

    argumentCheck(args.Length() == 0 || args.Length() == 1,
                  "NumberInt needs 0 or 1 arguments") if (args.Length() == 0) {
        it->SetHiddenValue(v8::String::NewFromUtf8(scope->getIsolate(), "__NumberInt"),
                           v8::Number::New(scope->getIsolate(), 0));
    }
    else if (args.Length() == 1) {
        it->SetHiddenValue(v8::String::NewFromUtf8(scope->getIsolate(), "__NumberInt"),
                           args[0]->ToInt32());
    }
    return it;
}

int numberIntVal(V8Scope* scope, const v8::Local<v8::Object>& it) {
    verify(scope->NumberIntFT()->HasInstance(it));
    v8::Local<v8::Value> value =
        it->GetHiddenValue(v8::String::NewFromUtf8(scope->getIsolate(), "__NumberInt"));
    verify(!value.IsEmpty());
    return value->Int32Value();
}

v8::Local<v8::Value> numberIntValueOf(V8Scope* scope,
                                      const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> it = args.This();
    return v8::Integer::New(scope->getIsolate(), numberIntVal(scope, it));
}

v8::Local<v8::Value> numberIntToNumber(V8Scope* scope,
                                       const v8::FunctionCallbackInfo<v8::Value>& args) {
    return numberIntValueOf(scope, args);
}

v8::Local<v8::Value> numberIntToString(V8Scope* scope,
                                       const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Object> it = args.This();
    int val = numberIntVal(scope, it);
    string ret = str::stream() << "NumberInt(" << val << ")";
    return v8::String::NewFromUtf8(scope->getIsolate(), ret.c_str());
}

v8::Local<v8::Value> v8ObjectInvalidForStorage(V8Scope* scope,
                                               const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 1, "invalidForStorage needs 1 argument") if (args[0]->IsNull()) {
        return v8::Null(scope->getIsolate());
    }
    argumentCheck(args[0]->IsObject(), "argument to invalidForStorage has to be an object")
        Status validForStorage = scope->v8ToMongo(args[0]->ToObject()).storageValid(true);
    if (validForStorage.isOK()) {
        return v8::Null(scope->getIsolate());
    }

    std::string errmsg = str::stream() << validForStorage.codeString() << ": "
                                       << validForStorage.reason();
    return v8::String::NewFromUtf8(scope->getIsolate(), errmsg.c_str());
}

v8::Local<v8::Value> bsonsize(V8Scope* scope, const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 1, "bsonsize needs 1 argument") if (args[0]->IsNull()) {
        return v8::Number::New(scope->getIsolate(), 0);
    }
    argumentCheck(args[0]->IsObject(), "argument to bsonsize has to be an object") return v8::
        Number::New(scope->getIsolate(), scope->v8ToMongo(args[0]->ToObject()).objsize());
}

v8::Local<v8::Value> bsonWoCompare(V8Scope* scope,
                                   const v8::FunctionCallbackInfo<v8::Value>& args) {
    argumentCheck(args.Length() == 2, "bsonWoCompare needs 2 argument");

    argumentCheck(args[0]->IsObject(), "first argument to bsonWoCompare has to be an object");
    argumentCheck(args[1]->IsObject(), "second argument to bsonWoCompare has to be an object");

    BSONObj firstObject(scope->v8ToMongo(args[0]->ToObject()));
    BSONObj secondObject(scope->v8ToMongo(args[1]->ToObject()));

    return v8::Number::New(scope->getIsolate(), firstObject.woCompare(secondObject));
}
}
