// mr.h

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
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the GNU Affero General Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#pragma once

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <string>
#include <vector>

#include "mongo/db/auth/privilege.h"
#include "mongo/db/curop.h"
#include "mongo/db/dbdirectclient.h"
#include "mongo/db/jsobj.h"
#include "mongo/platform/atomic_word.h"
#include "mongo/scripting/engine.h"

namespace mongo {

class Collection;
class Database;
class OperationContext;

namespace mr {

typedef std::vector<BSONObj> BSONList;

class State;

// ------------  function interfaces -----------

class Mapper : boost::noncopyable {
public:
    virtual ~Mapper() {}
    virtual void init(State* state) = 0;

    virtual void map(const BSONObj& o) = 0;
};

class Finalizer : boost::noncopyable {
public:
    virtual ~Finalizer() {}
    virtual void init(State* state) = 0;

    /**
     * this takes a tuple and returns a tuple
     */
    virtual BSONObj finalize(const BSONObj& tuple) = 0;
};

class Reducer : boost::noncopyable {
public:
    Reducer() : numReduces(0) {}
    virtual ~Reducer() {}
    virtual void init(State* state) = 0;

    virtual BSONObj reduce(const BSONList& tuples) = 0;
    /** this means its a final reduce, even if there is no finalizer */
    virtual BSONObj finalReduce(const BSONList& tuples, Finalizer* finalizer) = 0;

    long long numReduces;
};

// ------------  js function implementations -----------

/**
 * used as a holder for Scope and ScriptingFunction
 * visitor like pattern as Scope is gotten from first access
 */
class JSFunction : boost::noncopyable {
public:
    /**
     * @param type (map|reduce|finalize)
     */
    JSFunction(const std::string& type, const BSONElement& e);
    virtual ~JSFunction() {}

    virtual void init(State* state);

    Scope* scope() const {
        return _scope;
    }
    ScriptingFunction func() const {
        return _func;
    }

private:
    std::string _type;
    std::string _code;     // actual javascript code
    BSONObj _wantedScope;  // this is for CodeWScope

    Scope* _scope;  // this is not owned by us, and might be shared
    ScriptingFunction _func;
};

class JSMapper : public Mapper {
public:
    JSMapper(const BSONElement& code) : _func("_map", code) {}
    virtual void map(const BSONObj& o);
    virtual void init(State* state);

private:
    JSFunction _func;
    BSONObj _params;
};

class JSReducer : public Reducer {
public:
    JSReducer(const BSONElement& code) : _func("_reduce", code) {}
    virtual void init(State* state);

    virtual BSONObj reduce(const BSONList& tuples);
    virtual BSONObj finalReduce(const BSONList& tuples, Finalizer* finalizer);

private:
    /**
     * result in "__returnValue"
     * @param key OUT
     * @param endSizeEstimate OUT
    */
    void _reduce(const BSONList& values, BSONObj& key, int& endSizeEstimate);

    JSFunction _func;
};

class JSFinalizer : public Finalizer {
public:
    JSFinalizer(const BSONElement& code) : _func("_finalize", code) {}
    virtual BSONObj finalize(const BSONObj& o);
    virtual void init(State* state) {
        _func.init(state);
    }

private:
    JSFunction _func;
};

// -----------------


class TupleKeyCmp {
public:
    TupleKeyCmp() {}
    bool operator()(const BSONObj& l, const BSONObj& r) const {
        return l.firstElement().woCompare(r.firstElement()) < 0;
    }
};

typedef std::map<BSONObj, BSONList, TupleKeyCmp> InMemory;  // from key to list of tuples

/**
 * holds map/reduce config information
 */
class Config {
public:
    Config(const std::string& _dbname, const BSONObj& cmdObj);

    std::string dbname;
    std::string ns;

    // options
    bool verbose;
    bool jsMode;
    int splitInfo;

    // query options

    BSONObj filter;
    BSONObj sort;
    long long limit;

    // functions

    boost::scoped_ptr<Mapper> mapper;
    boost::scoped_ptr<Reducer> reducer;
    boost::scoped_ptr<Finalizer> finalizer;

    BSONObj mapParams;
    BSONObj scopeSetup;

    // output tables
    std::string incLong;
    std::string tempNamespace;

    enum OutputType {
        REPLACE,  // atomically replace the collection
        MERGE,    // merge keys, override dups
        REDUCE,   // merge keys, reduce dups
        INMEMORY  // only store in memory, limited in size
    };
    struct OutputOptions {
        std::string outDB;
        std::string collectionName;
        std::string finalNamespace;
        // if true, no lock during output operation
        bool outNonAtomic;
        OutputType outType;
    } outputOptions;

    static OutputOptions parseOutputOptions(const std::string& dbname, const BSONObj& cmdObj);

    // max number of keys allowed in JS map before switching mode
    long jsMaxKeys;
    // ratio of duplicates vs unique keys before reduce is triggered in js mode
    float reduceTriggerRatio;
    // maximum size of map before it gets dumped to disk
    long maxInMemSize;

    // true when called from mongos to do phase-1 of M/R
    bool shardedFirstPass;

    static AtomicUInt32 JOB_NUMBER;
};  // end MRsetup

/**
 * stores information about intermediate map reduce state
 * controls flow of data from map->reduce->finalize->output
 */
class State {
public:
    /**
     * txn must outlive this State.
     */
    State(OperationContext* txn, const Config& c);
    ~State();

    void init();

    // ---- prep  -----
    bool sourceExists();

    long long incomingDocuments();

    // ---- map stage ----

    /**
     * stages on in in-memory storage
     */
    void emit(const BSONObj& a);

    /**
    * Checks the size of the transient in-memory results accumulated so far and potentially
    * runs reduce in order to compact them. If the data is still too large, it will be
    * spilled to the output collection.
    *
    * NOTE: Make sure that no DB locks are held, when calling this function, because it may
    * try to acquire write DB lock for the write to the output collection.
    */
    void reduceAndSpillInMemoryStateIfNeeded();

    /**
     * run reduce on _temp
     */
    void reduceInMemory();

    /**
     * transfers in memory storage to temp collection
     */
    void dumpToInc();
    void insertToInc(BSONObj& o);
    void _insertToInc(BSONObj& o);

    // ------ reduce stage -----------

    void prepTempCollection();

    void finalReduce(BSONList& values);

    void finalReduce(CurOp* op, ProgressMeterHolder& pm);

    // ------- cleanup/data positioning ----------

    /**
     * Clean up the temporary and incremental collections
     */
    void dropTempCollections();

    /**
       @return number objects in collection
     */
    long long postProcessCollection(OperationContext* txn, CurOp* op, ProgressMeterHolder& pm);
    long long postProcessCollectionNonAtomic(OperationContext* txn,
                                             CurOp* op,
                                             ProgressMeterHolder& pm);

    /**
     * if INMEMORY will append
     * may also append stats or anything else it likes
     */
    void appendResults(BSONObjBuilder& b);

    // -------- util ------------

    /**
     * inserts with correct replication semantics
     */
    void insert(const std::string& ns, const BSONObj& o);

    // ------ simple accessors -----

    /** State maintains ownership, do no use past State lifetime */
    Scope* scope() {
        return _scope.get();
    }

    const Config& config() {
        return _config;
    }

    bool isOnDisk() {
        return _onDisk;
    }

    long long numEmits() const {
        if (_jsMode)
            return _scope->getNumberLongLong("_emitCt");
        return _numEmits;
    }
    long long numReduces() const {
        if (_jsMode)
            return _scope->getNumberLongLong("_redCt");
        return _config.reducer->numReduces;
    }
    long long numInMemKeys() const {
        if (_jsMode)
            return _scope->getNumberLongLong("_keyCt");
        return _temp->size();
    }

    bool jsMode() {
        return _jsMode;
    }
    void switchMode(bool jsMode);
    void bailFromJS();

    Collection* getCollectionOrUassert(Database* db, const StringData& ns);

    const Config& _config;
    DBDirectClient _db;
    bool _useIncremental;  // use an incremental collection

protected:
    /**
     * Appends a new document to the in-memory list of tuples, which are under that
     * document's key.
     *
     * @return estimated in-memory size occupied by the newly added document.
     */
    int _add(InMemory* im, const BSONObj& a);

    OperationContext* _txn;
    boost::scoped_ptr<Scope> _scope;
    bool _onDisk;  // if the end result of this map reduce is disk or not

    boost::scoped_ptr<InMemory> _temp;
    long _size;      // bytes in _temp
    long _dupCount;  // number of duplicate key entries

    long long _numEmits;

    bool _jsMode;
    ScriptingFunction _reduceAll;
    ScriptingFunction _reduceAndEmit;
    ScriptingFunction _reduceAndFinalize;
    ScriptingFunction _reduceAndFinalizeAndInsert;
};

BSONObj fast_emit(const BSONObj& args, void* data);
BSONObj _bailFromJS(const BSONObj& args, void* data);

void addPrivilegesRequiredForMapReduce(Command* commandTemplate,
                                       const std::string& dbname,
                                       const BSONObj& cmdObj,
                                       std::vector<Privilege>* out);
}  // end mr namespace
}
