// @file collection.js - DBCollection support in the mongo shell
// db.colName is a DBCollection object
// or db["colName"]

if ( ( typeof  DBCollection ) == "undefined" ){
    DBCollection = function( mongo , db , shortName , fullName ){
        this._mongo = mongo;
        this._db = db;
        this._shortName = shortName;
        this._fullName = fullName;

        this.verify();
    }
}

DBCollection.prototype.verify = function(){
    assert( this._fullName , "no fullName" );
    assert( this._shortName , "no shortName" );
    assert( this._db , "no db" );

    assert.eq( this._fullName , this._db._name + "." + this._shortName , "name mismatch" );

    assert( this._mongo , "no mongo in DBCollection" );
    assert( this.getMongo() , "no mongo from getMongo()" );
}

DBCollection.prototype.getName = function(){
    return this._shortName;
}

DBCollection.prototype.help = function () {
    var shortName = this.getName();
    print("DBCollection help");
    print("\tdb." + shortName + ".find().help() - show DBCursor help");
    print("\tdb." + shortName + ".count()");
    print("\tdb." + shortName + ".copyTo(newColl) - duplicates collection by copying all documents to newColl; no indexes are copied.");
    print("\tdb." + shortName + ".convertToCapped(maxBytes) - calls {convertToCapped:'" + shortName + "', size:maxBytes}} command");
    print("\tdb." + shortName + ".dataSize()");
    print("\tdb." + shortName + ".distinct( key ) - e.g. db." + shortName + ".distinct( 'x' )");
    print("\tdb." + shortName + ".drop() drop the collection");
    print("\tdb." + shortName + ".dropIndex(index) - e.g. db." + shortName + ".dropIndex( \"indexName\" ) or db." + shortName + ".dropIndex( { \"indexKey\" : 1 } )");
    print("\tdb." + shortName + ".dropIndexes()");
    print("\tdb." + shortName + ".ensureIndex(keypattern[,options])");
    print("\tdb." + shortName + ".explain().help() - show explain help");
    print("\tdb." + shortName + ".reIndex()");
    print("\tdb." + shortName + ".find([query],[fields]) - query is an optional query filter. fields is optional set of fields to return.");
    print("\t                                              e.g. db." + shortName + ".find( {x:77} , {name:1, x:1} )");
    print("\tdb." + shortName + ".find(...).count()");
    print("\tdb." + shortName + ".find(...).limit(n)");
    print("\tdb." + shortName + ".find(...).skip(n)");
    print("\tdb." + shortName + ".find(...).sort(...)");
    print("\tdb." + shortName + ".findOne([query])");
    print("\tdb." + shortName + ".findAndModify( { update : ... , remove : bool [, query: {}, sort: {}, 'new': false] } )");
    print("\tdb." + shortName + ".getDB() get DB object associated with collection");
    print("\tdb." + shortName + ".getPlanCache() get query plan cache associated with collection");
    print("\tdb." + shortName + ".getIndexes()");
    print("\tdb." + shortName + ".group( { key : ..., initial: ..., reduce : ...[, cond: ...] } )");
    // print("\tdb." + shortName + ".indexStats({expandNodes: [<expanded child numbers>}, <detailed: t/f>) - output aggregate/per-depth btree bucket stats");
    print("\tdb." + shortName + ".insert(obj)");
    print("\tdb." + shortName + ".mapReduce( mapFunction , reduceFunction , <optional params> )");
    print("\tdb." + shortName + ".aggregate( [pipeline], <optional params> ) - performs an aggregation on a collection; returns a cursor");
    print("\tdb." + shortName + ".remove(query)");
    print("\tdb." + shortName + ".renameCollection( newName , <dropTarget> ) renames the collection.");
    print("\tdb." + shortName + ".runCommand( name , <options> ) runs a db command with the given name where the first param is the collection name");
    print("\tdb." + shortName + ".save(obj)");
    print("\tdb." + shortName + ".stats({scale: N, indexDetails: true/false, " +
          "indexDetailsKey: <index key>, indexDetailsName: <index name>})");
    // print("\tdb." + shortName + ".diskStorageStats({[extent: <num>,] [granularity: <bytes>,] ...}) - analyze record layout on disk");
    // print("\tdb." + shortName + ".pagesInRAM({[extent: <num>,] [granularity: <bytes>,] ...}) - analyze resident memory pages");
    print("\tdb." + shortName + ".storageSize() - includes free space allocated to this collection");
    print("\tdb." + shortName + ".totalIndexSize() - size in bytes of all the indexes");
    print("\tdb." + shortName + ".totalSize() - storage allocated for all data and indexes");
    print("\tdb." + shortName + ".update(query, object[, upsert_bool, multi_bool]) - instead of two flags, you can pass an object with fields: upsert, multi");
    print("\tdb." + shortName + ".validate( <full> ) - SLOW");;
    // print("\tdb." + shortName + ".getIndexStats({expandNodes: [<expanded child numbers>}, <detailed: t/f>) - same as .indexStats but prints a human readable summary of the output");
    print("\tdb." + shortName + ".getShardVersion() - only for use with sharding");
    print("\tdb." + shortName + ".getShardDistribution() - prints statistics about data distribution in the cluster");
    print("\tdb." + shortName + ".getSplitKeysForChunks( <maxChunkSize> ) - calculates split points over all chunks and returns splitter function");
    print("\tdb." + shortName + ".getWriteConcern() - returns the write concern used for any operations on this collection, inherited from server/db if set");
    print("\tdb." + shortName + ".setWriteConcern( <write concern doc> ) - sets the write concern for writes to the collection");
    print("\tdb." + shortName + ".unsetWriteConcern( <write concern doc> ) - unsets the write concern for writes to the collection");
    // print("\tdb." + shortName + ".getDiskStorageStats({...}) - prints a summary of disk usage statistics");
    // print("\tdb." + shortName + ".getPagesInRAM({...}) - prints a summary of storage pages currently in physical memory");
    return __magicNoPrint;
}

DBCollection.prototype.getFullName = function(){
    return this._fullName;
}
DBCollection.prototype.getMongo = function(){
    return this._db.getMongo();
}
DBCollection.prototype.getDB = function(){
    return this._db;
}

DBCollection.prototype._dbCommand = function( cmd , params ){
    if ( typeof( cmd ) == "object" )
        return this._db._dbCommand( cmd );
    
    var c = {};
    c[cmd] = this.getName();
    if ( params )
        Object.extend( c , params );
    return this._db._dbCommand( c );    
}

DBCollection.prototype.runCommand = DBCollection.prototype._dbCommand;

DBCollection.prototype._massageObject = function( q ){
    if ( ! q )
        return {};

    var type = typeof q;

    if ( type == "function" )
        return { $where : q };

    if ( q.isObjectId )
        return { _id : q };

    if ( type == "object" )
        return q;

    if ( type == "string" ){
        if ( q.length == 24 )
            return { _id : q };

        return { $where : q };
    }

    throw Error( "don't know how to massage : " + type );

}


DBCollection.prototype._validateObject = function( o ){
    // Hidden property for testing purposes.
    if (this.getMongo()._skipValidation) return;

    if (typeof(o) != "object")
        throw Error( "attempted to save a " + typeof(o) + " value.  document expected." );

    if ( o._ensureSpecial && o._checkModify )
        throw Error( "can't save a DBQuery object" );
}

DBCollection._allowedFields = { $id : 1 , $ref : 1 , $db : 1 };

DBCollection.prototype._validateForStorage = function( o ){
    // Hidden property for testing purposes.
    if (this.getMongo()._skipValidation) return;

    this._validateObject( o );
    for ( var k in o ){
        if ( k.indexOf( "." ) >= 0 ) {
            throw Error( "can't have . in field names [" + k + "]" );
        }

        if ( k.indexOf( "$" ) == 0 && ! DBCollection._allowedFields[k] ) {
            throw Error( "field names cannot start with $ [" + k + "]" );
        }

        if ( o[k] !== null && typeof( o[k] ) === "object" ) {
            this._validateForStorage( o[k] );
        }
    }
};

DBCollection.prototype.find = function( query , fields , limit , skip, batchSize, options ){
    var cursor = new DBQuery( this._mongo , this._db , this ,
                        this._fullName , this._massageObject( query ) , fields , limit , skip , batchSize , options || this.getQueryOptions() );

    var connObj = this.getMongo();
    var readPrefMode = connObj.getReadPrefMode();
    if (readPrefMode != null) {
        cursor.readPref(readPrefMode, connObj.getReadPrefTagSet());
    }

    return cursor;
}

DBCollection.prototype.findOne = function( query , fields, options ){
    var cursor = this.find(query, fields, -1 /* limit */, 0 /* skip*/,
        0 /* batchSize */, options);

    if ( ! cursor.hasNext() )
        return null;
    var ret = cursor.next();
    if ( cursor.hasNext() ) throw Error( "findOne has more than 1 result!" );
    if ( ret.$err )
        throw Error( "error " + tojson( ret ) );
    return ret;
}

DBCollection.prototype.insert = function( obj , options, _allow_dot ){
    if ( ! obj )
        throw Error( "no object passed to insert!" );

    var flags = 0;
    
    var wc = undefined;
    var allowDottedFields = false;
    if ( options === undefined ) {
        // do nothing
    }
    else if ( typeof(options) == 'object' ) {
        if (options.ordered === undefined) {
            //do nothing, like above
        } else {
            flags = options.ordered ? 0 : 1;
        }
        
        if (options.writeConcern)
            wc = options.writeConcern;
        if (options.allowdotted)
            allowDottedFields = true;
    } else {
        flags = options;
    }
    
    // 1 = continueOnError, which is synonymous with unordered in the write commands/bulk-api
    var ordered = ((flags & 1) == 0);

    if (!wc)
        wc = this.getWriteConcern();

    var result = undefined;
    var startTime = (typeof(_verboseShell) === 'undefined' ||
                     !_verboseShell) ? 0 : new Date().getTime();

    if ( this.getMongo().writeMode() != "legacy" ) {
        // Bit 1 of option flag is continueOnError. Bit 0 (stop on error) is the default.
        var bulk = ordered ? this.initializeOrderedBulkOp() : this.initializeUnorderedBulkOp();
        var isMultiInsert = Array.isArray(obj);

        if (isMultiInsert) {
            obj.forEach(function(doc) {
                bulk.insert(doc);
            });
        }
        else {
            bulk.insert(obj);
        }

        try {
            result = bulk.execute(wc);
            if (!isMultiInsert)
                result = result.toSingleResult();
        }
        catch( ex ) {
            if ( ex instanceof BulkWriteError ) {
                result = isMultiInsert ? ex.toResult() : ex.toSingleResult();
            }
            else if ( ex instanceof WriteCommandError ) {
                result = isMultiInsert ? ex : ex.toSingleResult();
            }
            else {
                // Other exceptions thrown
                throw Error(ex);
            }
        }
    }
    else {
        if ( ! _allow_dot ) {
            this._validateForStorage( obj );
        }

        if ( typeof( obj._id ) == "undefined" && ! Array.isArray( obj ) ){
            var tmp = obj; // don't want to modify input
            obj = {_id: new ObjectId()};
            for (var key in tmp){
                obj[key] = tmp[key];
            }
        }

        this.getMongo().insert( this._fullName , obj, flags );

        // enforce write concern, if required
        if (wc)
            result = this.runCommand("getLastError", wc instanceof WriteConcern ? wc.toJSON() : wc);
    }

    this._lastID = obj._id;
    this._printExtraInfo("Inserted", startTime);
    return result;
};

DBCollection.prototype._validateRemoveDoc = function(doc) {
    // Hidden property for testing purposes.
    if (this.getMongo()._skipValidation) return;

    for (var k in doc) {
        if (k == "_id" && typeof( doc[k] ) == "undefined") {
          throw new Error("can't have _id set to undefined in a remove expression");
        }
    }
};

/**
 * Does validation of the remove args. Throws if the parse is not successful, otherwise
 * returns a document {query: <query>, justOne: <limit>, wc: <writeConcern>}.
 */
DBCollection.prototype._parseRemove = function( t , justOne ) {
    if (undefined === t) throw Error("remove needs a query");

    var query = this._massageObject(t);

    var wc = undefined;
    if (typeof(justOne) === "object") {
        var opts = justOne;
        wc = opts.writeConcern;
        justOne = opts.justOne;
    }

    // Normalize "justOne" to a bool.
    justOne = justOne ? true : false;

    // Handle write concern.
    if (!wc) {
        wc = this.getWriteConcern();
    }

    return {"query": query, "justOne": justOne, "wc": wc};
}

DBCollection.prototype.remove = function( t , justOne ){
    var parsed = this._parseRemove(t, justOne);
    var query = parsed.query;
    var justOne = parsed.justOne;
    var wc = parsed.wc;

    var result = undefined;
    var startTime = (typeof(_verboseShell) === 'undefined' ||
                     !_verboseShell) ? 0 : new Date().getTime();


    if ( this.getMongo().writeMode() != "legacy" ) {
        var bulk = this.initializeOrderedBulkOp();
        var removeOp = bulk.find(query);

        if (justOne) {
            removeOp.removeOne();
        }
        else {
            removeOp.remove();
        }

        try {
            result = bulk.execute(wc).toSingleResult();
        }
        catch( ex ) {
            if ( ex instanceof BulkWriteError || ex instanceof WriteCommandError ) {
                result = ex.toSingleResult();
            }
            else {
                // Other exceptions thrown
                throw Error(ex);
            }
        }
    }
    else {
        this._validateRemoveDoc(t);
        this.getMongo().remove(this._fullName, query, justOne );

        // enforce write concern, if required
        if (wc)
            result = this.runCommand("getLastError", wc instanceof WriteConcern ? wc.toJSON() : wc);

    }

    this._printExtraInfo("Removed", startTime);
    return result;
}

DBCollection.prototype._validateUpdateDoc = function(doc) {
    // Hidden property for testing purposes.
    if (this.getMongo()._skipValidation) return;

    var firstKey = null;
    for (var key in doc) { firstKey = key; break; }

    if (firstKey != null && firstKey[0] == '$') {
        // for mods we only validate partially, for example keys may have dots
        this._validateObject( doc );
    } else {
        // we're basically inserting a brand new object, do full validation
        this._validateForStorage( doc );
    }
};

/**
 * Does validation of the update args. Throws if the parse is not successful, otherwise
 * returns a document containing fields for query, obj, upsert, multi, and wc.
 *
 * Throws if the arguments are invalid.
 */
DBCollection.prototype._parseUpdate = function( query , obj , upsert , multi ){
    if (!query) throw Error("need a query");
    if (!obj) throw Error("need an object");

    var wc = undefined;
    // can pass options via object for improved readability
    if ( typeof(upsert) === "object" ) {
        if (multi) {
            throw Error("Fourth argument must be empty when specifying " +
                        "upsert and multi with an object.");
        }

        var opts = upsert;
        multi = opts.multi;
        wc = opts.writeConcern;
        upsert = opts.upsert;
    }

    // Normalize 'upsert' and 'multi' to booleans.
    upsert = upsert ? true : false;
    multi = multi ? true : false;

    if (!wc) {
        wc = this.getWriteConcern();
    }

    return {"query": query,
            "obj": obj,
            "upsert": upsert,
            "multi": multi,
            "wc": wc};
}

DBCollection.prototype.update = function( query , obj , upsert , multi ){
    var parsed = this._parseUpdate(query, obj, upsert, multi);
    var query = parsed.query;
    var obj = parsed.obj;
    var upsert = parsed.upsert;
    var multi = parsed.multi;
    var wc = parsed.wc;

    var result = undefined;
    var startTime = (typeof(_verboseShell) === 'undefined' ||
                     !_verboseShell) ? 0 : new Date().getTime();

    if ( this.getMongo().writeMode() != "legacy" ) {
        var bulk = this.initializeOrderedBulkOp();
        var updateOp = bulk.find(query);

        if (upsert) {
            updateOp = updateOp.upsert();
        }

        if (multi) {
            updateOp.update(obj);
        }
        else {
            updateOp.updateOne(obj);
        }

        try {
            result = bulk.execute(wc).toSingleResult();
        }
        catch( ex ) {
            if ( ex instanceof BulkWriteError || ex instanceof WriteCommandError ) {
                result = ex.toSingleResult();
            }
            else {
                // Other exceptions thrown
                throw Error(ex);
            }
        }
    }
    else {
        this._validateUpdateDoc(obj);
        this.getMongo().update(this._fullName, query, obj, upsert, multi);

        // enforce write concern, if required
        if (wc)
            result = this.runCommand("getLastError", wc instanceof WriteConcern ? wc.toJSON() : wc);
    }

    this._printExtraInfo("Updated", startTime);
    return result;
};

DBCollection.prototype.save = function( obj , opts ){
    if ( obj == null )
        throw Error("can't save a null");

    if ( typeof( obj ) == "number" || typeof( obj) == "string" )
        throw Error("can't save a number or string");

    if ( typeof( obj._id ) == "undefined" ){
        obj._id = new ObjectId();
        return this.insert( obj , opts );
    }
    else {
        return this.update( { _id : obj._id } , obj , Object.merge({ upsert:true }, opts));
    }
}

DBCollection.prototype._genIndexName = function( keys ){
    var name = "";
    for ( var k in keys ){
        var v = keys[k];
        if ( typeof v == "function" )
            continue;
        
        if ( name.length > 0 )
            name += "_";
        name += k + "_";

        name += v;
    }
    return name;
}

DBCollection.prototype._indexSpec = function( keys, options ) {
    var ret = { ns : this._fullName , key : keys , name : this._genIndexName( keys ) };

    if ( ! options ){
    }
    else if ( typeof ( options ) == "string" )
        ret.name = options;
    else if ( typeof ( options ) == "boolean" )
        ret.unique = true;
    else if ( typeof ( options ) == "object" ){
        if ( options.length ){
            var nb = 0;
            for ( var i=0; i<options.length; i++ ){
                if ( typeof ( options[i] ) == "string" )
                    ret.name = options[i];
                else if ( typeof( options[i] ) == "boolean" ){
                    if ( options[i] ){
                        if ( nb == 0 )
                            ret.unique = true;
                        if ( nb == 1 )
                            ret.dropDups = true;
                    }
                    nb++;
                }
            }
        }
        else {
            Object.extend( ret , options );
        }
    }
    else {
        throw Error( "can't handle: " + typeof( options ) );
    }

    return ret;
}

DBCollection.prototype.createIndex = function( keys , options ){
    var o = this._indexSpec( keys, options );

    if ( this.getMongo().writeMode() == "commands" ) {
        delete o.ns; // ns is passed to the first element in the command.
        return this._db.runCommand({ createIndexes: this.getName(), indexes: [o] });
    }
    else if( this.getMongo().writeMode() == "compatibility" ) {
        // Use the downconversion machinery of the bulk api to do a safe write, report response as a
        // command response
        var result = this._db.getCollection( "system.indexes" ).insert( o , 0, true );

        if (result.hasWriteError() || result.hasWriteConcernError()) {
            var error = result.hasWriteError() ? result.getWriteError() :
                                                 result.getWriteConcernError();
            return { ok : 0.0, code : error.code, errmsg : error.errmsg };
        }
        else {
            return { ok : 1.0 };
        }
    }
    else {
        this._db.getCollection( "system.indexes" ).insert( o , 0, true );
    }
}

DBCollection.prototype.ensureIndex = function( keys , options ){
    var result = this.createIndex(keys, options);

    if ( this.getMongo().writeMode() != "legacy" ) {
        return result;
    }

    err = this.getDB().getLastErrorObj();
    if (err.err) {
        return err;
    }
    // nothing returned on success
}

DBCollection.prototype.reIndex = function() {
    return this._db.runCommand({ reIndex: this.getName() });
}

DBCollection.prototype.dropIndexes = function(){
    if ( arguments.length )
        throw Error("dropIndexes doesn't take arguments");

    var res = this._db.runCommand( { deleteIndexes: this.getName(), index: "*" } );
    assert( res , "no result from dropIndex result" );
    if ( res.ok )
        return res;

    if ( res.errmsg.match( /not found/ ) )
        return res;

    throw Error( "error dropping indexes : " + tojson( res ) );
}


DBCollection.prototype.drop = function(){
    if ( arguments.length > 0 )
        throw Error("drop takes no argument");
    var ret = this._db.runCommand( { drop: this.getName() } );
    if ( ! ret.ok ){
        if ( ret.errmsg == "ns not found" )
            return false;
        throw Error( "drop failed: " + tojson( ret ) );
    }
    return true;
}

DBCollection.prototype.findAndModify = function(args){
    var cmd = { findandmodify: this.getName() };
    for (var key in args){
        cmd[key] = args[key];
    }

    var ret = this._db.runCommand( cmd );
    if ( ! ret.ok ){
        if (ret.errmsg == "No matching object found"){
            return null;
        }
        throw Error( "findAndModifyFailed failed: " + tojson( ret ) );
    }
    return ret.value;
}

DBCollection.prototype.renameCollection = function( newName , dropTarget ){
    return this._db._adminCommand( { renameCollection : this._fullName , 
                                     to : this._db._name + "." + newName , 
                                     dropTarget : dropTarget } )
}

// Display verbose information about the operation
DBCollection.prototype._printExtraInfo = function(action, startTime) {
    if ( typeof _verboseShell === 'undefined' || !_verboseShell ) {
        __callLastError = true;
        return;
    }

    // explicit w:1 so that replset getLastErrorDefaults aren't used here which would be bad.
    var res = this._db.getLastErrorCmd(1);
    if (res) {
        if (res.err != undefined && res.err != null) {
            // error occurred, display it
            print(res.err);
            return;
        }

        var info = action + " ";
        // hack for inserted because res.n is 0
        info += action != "Inserted" ? res.n : 1;
        if (res.n > 0 && res.updatedExisting != undefined)
            info += " " + (res.updatedExisting ? "existing" : "new")
        info += " record(s)";
        var time = new Date().getTime() - startTime;
        info += " in " + time + "ms";
        print(info);
    }
}

DBCollection.prototype.validate = function(full) {
    var cmd = { validate: this.getName() };

    if (typeof(full) == 'object') // support arbitrary options here
        Object.extend(cmd, full);
    else
        cmd.full = full;

    var res = this._db.runCommand( cmd );

    if (typeof(res.valid) == 'undefined') {
        // old-style format just put everything in a string. Now using proper fields

        res.valid = false;

        var raw = res.result || res.raw;

        if ( raw ){
            var str = "-" + tojson( raw );
            res.valid = ! ( str.match( /exception/ ) || str.match( /corrupt/ ) );

            var p = /lastExtentSize:(\d+)/;
            var r = p.exec( str );
            if ( r ){
                res.lastExtentSize = Number( r[1] );
            }
        }
    }

    return res;
}

/**
 * Invokes the storageDetails command to provide aggregate and (if requested) detailed information
 * regarding the layout of records and deleted records in the collection extents.
 * getDiskStorageStats provides a human-readable summary of the command output
 */
DBCollection.prototype.diskStorageStats = function(opt) {
    var cmd = { storageDetails: this.getName(), analyze: 'diskStorage' };
    if (typeof(opt) == 'object') Object.extend(cmd, opt);

    var res = this._db.runCommand(cmd);
    if (!res.ok && res.errmsg.match(/no such cmd/)) {
        print("this command requires starting mongod with --enableExperimentalStorageDetailsCmd");
    }
    return res;
}

// Refer to diskStorageStats
DBCollection.prototype.getDiskStorageStats = function(params) {
    var stats = this.diskStorageStats(params);
    if (!stats.ok) {
        print("error executing storageDetails command: " + stats.errmsg);
        return;
    }

    print("\n    " + "size".pad(9) + " " + "# recs".pad(10) + " " +
          "[===occupied by BSON=== ---occupied by padding---       free           ]" + "  " +
          "bson".pad(8) + " " + "rec".pad(8) + " " + "padding".pad(8));
    print();

    var BAR_WIDTH = 70;

    var formatSliceData = function(data) {
        var bar = _barFormat([
            [data.bsonBytes / data.onDiskBytes, "="],
            [(data.recBytes - data.bsonBytes) / data.onDiskBytes, "-"]
        ], BAR_WIDTH);

        return sh._dataFormat(data.onDiskBytes).pad(9) + " " +
               data.numEntries.toFixed(0).pad(10) + " " +
               bar + "  " +
               (data.bsonBytes / data.onDiskBytes).toPercentStr().pad(8) + " " +
               (data.recBytes / data.onDiskBytes).toPercentStr().pad(8) + " " +
               (data.recBytes / data.bsonBytes).toFixed(4).pad(8);
    };

    var printExtent = function(ex, rng) {
        print("--- extent " + rng + " ---");
        print("tot " + formatSliceData(ex));
        print();
        if (ex.slices) {
            for (var c = 0; c < ex.slices.length; c++) {
                var slice = ex.slices[c];
                print(("" + c).pad(3) + " " + formatSliceData(slice));
            }
            print();
        }
    };

    if (stats.extents) {
        print("--- extent overview ---\n");
        for (var i = 0; i < stats.extents.length; i++) {
            var ex = stats.extents[i];
            print(("" + i).pad(3) + " " + formatSliceData(ex));
        }
        print();
        if (params && (params.granularity || params.numberOfSlices)) {
            for (var i = 0; i < stats.extents.length; i++) {
                printExtent(stats.extents[i], i);
            }
        }
    } else {
        printExtent(stats, "range " + stats.range);
    }

}

/**
 * Invokes the storageDetails command to report the percentage of virtual memory pages of the
 * collection storage currently in physical memory (RAM).
 * getPagesInRAM provides a human-readable summary of the command output
 */
DBCollection.prototype.pagesInRAM = function(opt) {
    var cmd = { storageDetails: this.getName(), analyze: 'pagesInRAM' };
    if (typeof(opt) == 'object') Object.extend(cmd, opt);

    var res = this._db.runCommand(cmd);
    if (!res.ok && res.errmsg.match(/no such cmd/)) {
        print("this command requires starting mongod with --enableExperimentalStorageDetailsCmd");
    }
    return res;
}

// Refer to pagesInRAM
DBCollection.prototype.getPagesInRAM = function(params) {
    var stats = this.pagesInRAM(params);
    if (!stats.ok) {
        print("error executing storageDetails command: " + stats.errmsg);
        return;
    }

    var BAR_WIDTH = 70;
    var formatExtentData = function(data) {
        return "size".pad(8) + " " +
               _barFormat([ [data.inMem, '='] ], BAR_WIDTH) + "  " +
               data.inMem.toPercentStr().pad(7);
    }

    var printExtent = function(ex, rng) {
        print("--- extent " + rng + " ---");
        print("tot " + formatExtentData(ex));
        print();
        if (ex.slices) {
            print("\tslices, percentage of pages in memory (< .1% : ' ', <25% : '.', " +
                  "<50% : '_', <75% : '=', >75% : '#')");
            print();
            print("\t" + "offset".pad(8) + "  [slices...] (each slice is " +
                  sh._dataFormat(ex.sliceBytes) + ")");
            line = "\t" + ("" + 0).pad(8) + "  [";
            for (var c = 0; c < ex.slices.length; c++) {
                if (c % 80 == 0 && c != 0) {
                    print(line + "]");
                    line = "\t" + sh._dataFormat(ex.sliceBytes * c).pad(8) + "  [";
                }
                var inMem = ex.slices[c];
                if (inMem <= .001) line += " ";
                else if (inMem <= .25) line += ".";
                else if (inMem <= .5) line += "_";
                else if (inMem <= .75) line += "=";
                else line += "#";
            }
            print(line + "]");
            print();
        }
    }

    if (stats.extents) {
        print("--- extent overview ---\n");
        for (var i = 0; i < stats.extents.length; i++) {
            var ex = stats.extents[i];
            print(("" + i).pad(3) + " " + formatExtentData(ex));
        }
        print();
        if (params && (params.granularity || params.numberOfSlices)) {
            for (var i = 0; i < stats.extents.length; i++) {
                printExtent(stats.extents[i], i);
            }
        } else {
            print("use getPagesInRAM({granularity: _bytes_}) or " +
                  "getPagesInRAM({numberOfSlices: _num_} for details");
            print("use pagesInRAM(...) for json output, same parameters apply");
        }
    } else {
        printExtent(stats, "range " + stats.range);
    }
}

DBCollection.prototype.indexStats = function(params) {
    var cmd = { indexStats: this.getName() };

    if (typeof(params) == 'object') // support arbitrary options here
        Object.extend(cmd, params);

    var res = this._db.runCommand(cmd);
    if (!res.ok && res.errmsg.match(/no such cmd/)) {
        print("this command requires starting mongod with --enableExperimentalIndexStatsCmd");
    }
    return res;
}

DBCollection.prototype.getIndexStats = function(params, detailed) {
    var stats = this.indexStats(params);
    if (!stats.ok) {
        print("error executing indexStats command: " + tojson(stats));
        return;
    }

    print("-- index \"" + stats.index + "\" --");
    print("  version " + stats.version + " | key pattern " +
          tojsononeline(stats.keyPattern) + (stats.isIdIndex ? " [id index]" : "") +
          " | storage namespace \"" + stats.storageNs + "\"");
    print("  " + stats.depth + " deep, bucket body is " + stats.bucketBodyBytes + " bytes");
    print();
    if (detailed) {
        print("  **  min |-- .02 quant --- 1st quartile [=== median ===] 3rd quartile --- " +
              ".98 quant --| max  **  ");
        print();
    }

    // format a number rounding to three decimal figures
    var fnum = function(n) {
        return n.toFixed(3);
    }

    var formatBoxPlot = function(st) {
        var out = "";
        if (st.count == 0) return "no samples";
        out += "avg. " + fnum(st.mean);
        if (st.count == 1) return out;
        out += " | stdev. " + fnum(st.stddev);

        var quant = function(st, prob) {
            return st.quantiles["" + prob].toFixed(3);
        }
        if (st.quantiles) {
            out += "\t" + fnum(st.min) + " |-- " + quant(st, 0.02) + " --- " + quant(st, 0.25) +
                   " [=== " + quant(st, 0.5) + " ===] " + quant(st, 0.75) + " --- " +
                   quant(st, 0.98) + " --| " + fnum(st.max) + " ";
        }
        return out;
    }

    var formatStats = function(indent, nd) {
        var out = "";
        out += indent + "bucket count\t" + nd.numBuckets
                      + "\ton average " + fnum(nd.fillRatio.mean * 100) + " %"
                      + " (±" + fnum((nd.fillRatio.stddev) * 100) + " %) full"
                      + "\t" + fnum(nd.bsonRatio.mean * 100) + " %"
                      + " (±" + fnum((nd.bsonRatio.stddev) * 100) + " %) bson keys, "
                      + fnum(nd.keyNodeRatio.mean * 100) + " %"
                      + " (±" + fnum((nd.keyNodeRatio.stddev) * 100) + " %) key nodes\n";
        if (detailed) {
            out += indent + "\n";
            out += indent + "key count\t" + formatBoxPlot(nd.keyCount) + "\n";
            out += indent + "used keys\t" + formatBoxPlot(nd.usedKeyCount) + "\n";
            out += indent + "space occupied by (ratio of bucket)\n";
            out += indent + "  key nodes\t" + formatBoxPlot(nd.keyNodeRatio) + "\n";
            out += indent + "  key objs \t" + formatBoxPlot(nd.bsonRatio) + "\n";
            out += indent + "  used     \t" + formatBoxPlot(nd.fillRatio) + "\n";
        }
        return out;
    }

    print(formatStats("  ", stats.overall));
    print();

    for (var d = 0; d <= stats.depth; ++d) {
        print("  -- depth " + d + " --");
        print(formatStats("    ", stats.perLevel[d]));
    }

    if (stats.expandedNodes) {
        print("\n-- expanded nodes --\n");
        for (var d = 0; d < stats.expandedNodes.length - 1; ++d) {
            var node;
            if (d == 0) {
                node = stats.expandedNodes[0][0];
                print("  -- root -- ");
            } else {
                node = stats.expandedNodes[d][params.expandNodes[d]];
                print("  -- node # " + params.expandNodes[d] + " at depth " +
                      node.nodeInfo.depth + " -- ");
            }
            print("    " + (node.nodeInfo.firstKey ? tojsononeline(node.nodeInfo.firstKey) : "") + " -> " +
                  (node.nodeInfo.lastKey ? tojsononeline(node.nodeInfo.lastKey) : ""));
            print("    " + node.nodeInfo.keyCount + " keys (" + node.nodeInfo.keyCount + " used)" +
                  "\tat diskloc " + tojsononeline(node.nodeInfo.diskLoc));
            print("    ");
            print("    subtree stats, excluding node");
            print(formatStats("      ", node));

            if (detailed) {
                print("    children (: % full, subtree % full)");
                var children = "      ";
                for (var k = 0; k < stats.expandedNodes[d + 1].length; ++k) {
                    var node = stats.expandedNodes[d + 1][k];
                    if (node.nodeInfo != undefined) {
                        children += node.nodeInfo.childNum + ": " +
                                    (node.nodeInfo.fillRatio * 100).toFixed(1) + ", " +
                                    (node.fillRatio.mean * 100).toFixed(1) + " | ";
                    } else {
                        children += k + ": - | ";
                    }
                    if (k != 0 && k % 5 == 0) children += "\n      ";
                }
                print(children);
                print(" ");
            }
        }
    }
}

DBCollection.prototype.getShardVersion = function(){
    return this._db._adminCommand( { getShardVersion : this._fullName } );
}

DBCollection.prototype._getIndexesSystemIndexes = function(filter){
    var si = this.getDB().getCollection( "system.indexes" );
    var query = { ns : this.getFullName() };
    if (filter)
        query = Object.extend(query, filter)
    return si.find( query ).toArray();
}

DBCollection.prototype._getIndexesCommand = function(filter){
    var res = this.runCommand( "listIndexes", filter );

    if ( !res.ok ) {

        if ( res.code == 59 ) {
            // command doesn't exist, old mongod
            return null;
        }

        if ( res.code == 26 ) {
            // NamespaceNotFound, for compatability, return []
            return [];
        }

        if ( res.errmsg && res.errmsg.startsWith( "no such cmd" ) ) {
            return null;
        }

        throw Error( "listIndexes failed: " + tojson( res ) );
    }

    return new DBCommandCursor(this._mongo, res).toArray();
}

DBCollection.prototype.getIndexes = function(filter){
    var res = this._getIndexesCommand(filter);
    if ( res ) {
        return res;
    }
    return this._getIndexesSystemIndexes(filter);
}

DBCollection.prototype.getIndices = DBCollection.prototype.getIndexes;
DBCollection.prototype.getIndexSpecs = DBCollection.prototype.getIndexes;

DBCollection.prototype.getIndexKeys = function(){
    return this.getIndexes().map(
        function(i){
            return i.key;
        }
    );
}


DBCollection.prototype.count = function( x ){
    return this.find( x ).count();
}

DBCollection.prototype.hashAllDocs = function() {
    var cmd = { dbhash : 1,
                collections : [ this._shortName ] };
    var res = this._dbCommand( cmd );
    var hash = res.collections[this._shortName];
    assert( hash );
    assert( typeof(hash) == "string" );
    return hash;
}

/**
 * <p>Drop a specified index.</p>
 *
 * <p>
 * "index" is the name of the index in the system.indexes name field (run db.system.indexes.find() to
 *  see example data), or an object holding the key(s) used to create the index.
 * For example:
 *  db.collectionName.dropIndex( "myIndexName" );
 *  db.collectionName.dropIndex( { "indexKey" : 1 } );
 * </p>
 *
 * @param {String} name or key object of index to delete.
 * @return A result object.  result.ok will be true if successful.
 */
DBCollection.prototype.dropIndex =  function(index) {
    assert(index, "need to specify index to dropIndex" );
    var res = this._dbCommand( "deleteIndexes", { index: index } );
    return res;
}

DBCollection.prototype.copyTo = function( newName ){
    return this.getDB().eval(
        function( collName , newName ){
            var from = db[collName];
            var to = db[newName];
            to.ensureIndex( { _id : 1 } );
            var count = 0;

            var cursor = from.find();
            while ( cursor.hasNext() ){
                var o = cursor.next();
                count++;
                to.save( o );
            }

            return count;
        } , this.getName() , newName
    );
}

DBCollection.prototype.getCollection = function( subName ){
    return this._db.getCollection( this._shortName + "." + subName );
}

/**
  * scale: The scale at which to deliver results. Unless specified, this command returns all data
  *        in bytes.
  * indexDetails: Includes indexDetails field in results. Default: false.
  * indexDetailsKey: If indexDetails is true, filter contents in indexDetails by this index key.
  * indexDetailsname: If indexDetails is true, filter contents in indexDetails by this index name.
  *
  * It is an error to provide both indexDetailsKey and indexDetailsName.
  */
DBCollection.prototype.stats = function(args) {
    'use strict';

    // For backwards compatibility with db.collection.stats(scale).
    var scale = isObject(args) ? args.scale : args;

    var options = isObject(args) ? args : {};
    if (options.indexDetailsKey && options.indexDetailsName) {
        throw new Error('Cannot filter indexDetails on both indexDetailsKey and ' +
                        'indexDetailsName');
    }

    var res = this._db.runCommand({collStats: this._shortName, scale: scale});
    if (!res.ok) {
        return res;
    }

    var getIndexName = function(collection, indexKey) {
        if (!isObject(indexKey)) return undefined;
        var indexName;
        collection.getIndexes().forEach(function(spec) {
            if (friendlyEqual(spec.key, options.indexDetailsKey)) {
                indexName = spec.name;
            }
        });
        return indexName;
    };

    var filterIndexName =
        options.indexDetailsName || getIndexName(this, options.indexDetailsKey);

    var updateStats = function(stats, keepIndexDetails, indexName) {
        if (!stats.indexDetails) return;
        if (!keepIndexDetails) {
            delete stats.indexDetails;
            return;
        }
        if (!indexName) return;
        for (var key in stats.indexDetails) {
            if (key == indexName) continue;
            delete stats.indexDetails[key];
        }
    };

    updateStats(res, options.indexDetails, filterIndexName);

    if (res.sharded) {
        for (var shardName in res.shards) {
            updateStats(res.shards[shardName], options.indexDetails, filterIndexName);
        }
    }

    return res;
}

DBCollection.prototype.dataSize = function(){
    return this.stats().size;
}

DBCollection.prototype.storageSize = function(){
    return this.stats().storageSize;
}

DBCollection.prototype.totalIndexSize = function( verbose ){
    var stats = this.stats();
    if (verbose){
        for (var ns in stats.indexSizes){
            print( ns + "\t" + stats.indexSizes[ns] );
        }
    }
    return stats.totalIndexSize;
}


DBCollection.prototype.totalSize = function(){
    var total = this.storageSize();
    var totalIndexSize = this.totalIndexSize();
    if (totalIndexSize) {
        total += totalIndexSize;
    }
    return total;
}


DBCollection.prototype.convertToCapped = function( bytes ){
    if ( ! bytes )
        throw Error("have to specify # of bytes");
    return this._dbCommand( { convertToCapped : this._shortName , size : bytes } )
}

DBCollection.prototype.exists = function(){
    var res = this._db.runCommand( "listCollections",
                                  { filter : { name : this._shortName } } );
    if ( res.ok ) {
        var cursor = new DBCommandCursor( this._mongo, res );
        if ( !cursor.hasNext() )
            return null;
        return cursor.next();
    }

    if ( res.errmsg && res.errmsg.startsWith( "no such cmd" ) ) {
        return this._db.system.namespaces.findOne( { name : this._fullName } );
    }

    throw Error( "listCollections failed: " + tojson( res ) );
}

DBCollection.prototype.isCapped = function(){
    var e = this.exists();
    return ( e && e.options && e.options.capped ) ? true : false;
}

DBCollection.prototype._distinct = function( keyString , query ){
    return this._dbCommand( { distinct : this._shortName , key : keyString , query : query || {} } );
}

DBCollection.prototype.distinct = function( keyString , query ){
    keyStringType = typeof keyString;
    if (keyStringType != "string")
        throw Error("The first argument to the distinct command must be a string but was a " + keyStringType);
    queryType = typeof query;
    if (query != null && queryType != "object")
        throw Error("The query argument to the distinct command must be a document but was a " + queryType);
    var res = this._distinct( keyString , query );
    if ( ! res.ok )
        throw Error("distinct failed: " + tojson( res ));
    return res.values;
}


DBCollection.prototype.aggregate = function(pipeline, extraOpts) {
    if (!(pipeline instanceof Array)) {
        // support legacy varargs form. (Also handles db.foo.aggregate())
        pipeline = argumentsToArray(arguments)
        extraOpts = {}
    }
    else if (extraOpts === undefined) {
        extraOpts = {};
    }

    var cmd = {pipeline: pipeline};
    Object.extend(cmd, extraOpts);

    if (!('cursor' in cmd)) {
        // implicitly use cursors
        cmd.cursor = {};
    }

    var res = this.runCommand("aggregate", cmd);

    if (!res.ok
            && (res.code == 17020 || res.errmsg == "unrecognized field \"cursor")
            && !("cursor" in extraOpts)) {
        // If the command failed because cursors aren't supported and the user didn't explicitly
        // request a cursor, try again without requesting a cursor.
        delete cmd.cursor;
        res = this.runCommand("aggregate", cmd);

        if ('result' in res && !("cursor" in res)) {
            // convert old-style output to cursor-style output
            res.cursor = {ns: '', id: NumberLong(0)};
            res.cursor.firstBatch = res.result;
            delete res.result;
        }
    }

    assert.commandWorked(res, "aggregate failed");

    if ("cursor" in res)
        return new DBCommandCursor(this._mongo, res);

    return res;
}

DBCollection.prototype.group = function( params ){
    params.ns = this._shortName;
    return this._db.group( params );
}

DBCollection.prototype.groupcmd = function( params ){
    params.ns = this._shortName;
    return this._db.groupcmd( params );
}

MapReduceResult = function( db , o ){
    Object.extend( this , o );
    this._o = o;
    this._keys = Object.keySet( o );
    this._db = db;
    if ( this.result != null ) {
        this._coll = this._db.getCollection( this.result );
    }
}

MapReduceResult.prototype._simpleKeys = function(){
    return this._o;
}

MapReduceResult.prototype.find = function(){
    if ( this.results )
        return this.results;
    return DBCollection.prototype.find.apply( this._coll , arguments );
}

MapReduceResult.prototype.drop = function(){
    if ( this._coll ) {
        return this._coll.drop();
    }
}

/**
* just for debugging really
*/
MapReduceResult.prototype.convertToSingleObject = function(){
    var z = {};
    var it = this.results != null ? this.results : this._coll.find();
    it.forEach( function(a){ z[a._id] = a.value; } );
    return z;
}

DBCollection.prototype.convertToSingleObject = function(valueField){
    var z = {};
    this.find().forEach( function(a){ z[a._id] = a[valueField]; } );
    return z;
}

/**
* @param optional object of optional fields;
*/
DBCollection.prototype.mapReduce = function( map , reduce , optionsOrOutString ){
    var c = { mapreduce : this._shortName , map : map , reduce : reduce };
    assert( optionsOrOutString , "need to supply an optionsOrOutString" )

    if ( typeof( optionsOrOutString ) == "string" )
        c["out"] = optionsOrOutString;
    else
        Object.extend( c , optionsOrOutString );

    var raw = this._db.runCommand( c );
    if ( ! raw.ok ){
        __mrerror__ = raw;
        throw Error( "map reduce failed:" + tojson(raw) );
    }
    return new MapReduceResult( this._db , raw );

}

DBCollection.prototype.toString = function(){
    return this.getFullName();
}

DBCollection.prototype.toString = function(){
    return this.getFullName();
}


DBCollection.prototype.tojson = DBCollection.prototype.toString;

DBCollection.prototype.shellPrint = DBCollection.prototype.toString;

DBCollection.autocomplete = function(obj){
    var colls = DB.autocomplete(obj.getDB());
    var ret = [];
    for (var i=0; i<colls.length; i++){
        var c = colls[i];
        if (c.length <= obj.getName().length) continue;
        if (c.slice(0,obj.getName().length+1) != obj.getName()+'.') continue;

        ret.push(c.slice(obj.getName().length+1));
    }
    return ret;
}


// Sharding additions

/* 
Usage :

mongo <mongos>
> load('path-to-file/shardingAdditions.js')
Loading custom sharding extensions...
true

> var collection = db.getMongo().getCollection("foo.bar")
> collection.getShardDistribution() // prints statistics related to the collection's data distribution

> collection.getSplitKeysForChunks() // generates split points for all chunks in the collection, based on the
                                     // default maxChunkSize or alternately a specified chunk size
> collection.getSplitKeysForChunks( 10 ) // Mb

> var splitter = collection.getSplitKeysForChunks() // by default, the chunks are not split, the keys are just
                                                    // found.  A splitter function is returned which will actually
                                                    // do the splits.
                                                    
> splitter() // ! Actually executes the splits on the cluster !
                                                    
*/

DBCollection.prototype.getShardDistribution = function(){

   var stats = this.stats()
   
   if( ! stats.sharded ){
       print( "Collection " + this + " is not sharded." )
       return
   }
   
   var config = this.getMongo().getDB("config")
       
   var numChunks = 0
   
   for( var shard in stats.shards ){
       
       var shardDoc = config.shards.findOne({ _id : shard })
       
       print( "\nShard " + shard + " at " + shardDoc.host ) 
       
       var shardStats = stats.shards[ shard ]
               
       var chunks = config.chunks.find({ _id : sh._collRE( this ), shard : shard }).toArray()
       
       numChunks += chunks.length
       
       var estChunkData = shardStats.size / chunks.length
       var estChunkCount = Math.floor( shardStats.count / chunks.length )
       
       print( " data : " + sh._dataFormat( shardStats.size ) +
              " docs : " + shardStats.count +
              " chunks : " +  chunks.length )
       print( " estimated data per chunk : " + sh._dataFormat( estChunkData ) )
       print( " estimated docs per chunk : " + estChunkCount )
       
   }
   
   print( "\nTotals" )
   print( " data : " + sh._dataFormat( stats.size ) +
          " docs : " + stats.count +
          " chunks : " +  numChunks )
   for( var shard in stats.shards ){
   
       var shardStats = stats.shards[ shard ]
       
       var estDataPercent = Math.floor( shardStats.size / stats.size * 10000 ) / 100
       var estDocPercent = Math.floor( shardStats.count / stats.count * 10000 ) / 100
       
       print( " Shard " + shard + " contains " + estDataPercent + "% data, " + estDocPercent + "% docs in cluster, " +
              "avg obj size on shard : " + sh._dataFormat( stats.shards[ shard ].avgObjSize ) )
   }
   
   print( "\n" )
   
}


DBCollection.prototype.getSplitKeysForChunks = function( chunkSize ){
       
   var stats = this.stats()
   
   if( ! stats.sharded ){
       print( "Collection " + this + " is not sharded." )
       return
   }
   
   var config = this.getMongo().getDB("config")
   
   if( ! chunkSize ){
       chunkSize = config.settings.findOne({ _id : "chunksize" }).value
       print( "Chunk size not set, using default of " + chunkSize + "MB" )
   }
   else{
       print( "Using chunk size of " + chunkSize + "MB" )
   }
    
   var shardDocs = config.shards.find().toArray()
   
   var allSplitPoints = {}
   var numSplits = 0    
   
   for( var i = 0; i < shardDocs.length; i++ ){
       
       var shardDoc = shardDocs[i]
       var shard = shardDoc._id
       var host = shardDoc.host
       var sconn = new Mongo( host )
       
       var chunks = config.chunks.find({ _id : sh._collRE( this ), shard : shard }).toArray()
       
       print( "\nGetting split points for chunks on shard " + shard + " at " + host )
               
       var splitPoints = []
       
       for( var j = 0; j < chunks.length; j++ ){
           var chunk = chunks[j]
           var result = sconn.getDB("admin").runCommand({ splitVector : this + "", min : chunk.min, max : chunk.max, maxChunkSize : chunkSize })
           if( ! result.ok ){
               print( " Had trouble getting split keys for chunk " + sh._pchunk( chunk ) + " :\n" )
               printjson( result )
           }
           else{
               splitPoints = splitPoints.concat( result.splitKeys )
               
               if( result.splitKeys.length > 0 )
                   print( " Added " + result.splitKeys.length + " split points for chunk " + sh._pchunk( chunk ) )
           }
       }
       
       print( "Total splits for shard " + shard + " : " + splitPoints.length )
       
       numSplits += splitPoints.length
       allSplitPoints[ shard ] = splitPoints
       
   }
   
   // Get most recent migration
   var migration = config.changelog.find({ what : /^move.*/ }).sort({ time : -1 }).limit( 1 ).toArray()
   if( migration.length == 0 ) 
       print( "\nNo migrations found in changelog." )
   else {
       migration = migration[0]
       print( "\nMost recent migration activity was on " + migration.ns + " at " + migration.time )
   }
   
   var admin = this.getMongo().getDB("admin") 
   var coll = this
   var splitFunction = function(){
       
       // Turn off the balancer, just to be safe
       print( "Turning off balancer..." )
       config.settings.update({ _id : "balancer" }, { $set : { stopped : true } }, true )
       print( "Sleeping for 30s to allow balancers to detect change.  To be extra safe, check config.changelog" +
              " for recent migrations." )
       sleep( 30000 )
              
       for( shard in allSplitPoints ){
           for( var i = 0; i < allSplitPoints[ shard ].length; i++ ){
               var splitKey = allSplitPoints[ shard ][i]
               print( "Splitting at " + tojson( splitKey ) )
               printjson( admin.runCommand({ split : coll + "", middle : splitKey }) )
           }
       }
       
       print( "Turning the balancer back on." )
       config.settings.update({ _id : "balancer" }, { $set : { stopped : false } } )
       sleep( 1 )
   }
   
   splitFunction.getSplitPoints = function(){ return allSplitPoints; }
   
   print( "\nGenerated " + numSplits + " split keys, run output function to perform splits.\n" +
          " ex : \n" + 
          "  > var splitter = <collection>.getSplitKeysForChunks()\n" +
          "  > splitter() // Execute splits on cluster !\n" )
       
   return splitFunction
   
}

DBCollection.prototype.setSlaveOk = function( value ) {
    if( value == undefined ) value = true;
    this._slaveOk = value;
}

DBCollection.prototype.getSlaveOk = function() {
    if (this._slaveOk != undefined) return this._slaveOk;
    return this._db.getSlaveOk();
}

DBCollection.prototype.getQueryOptions = function() {
    var options = 0;
    if (this.getSlaveOk()) options |= 4;
    return options;
}

/**
 * Returns a PlanCache for the collection.
 */
DBCollection.prototype.getPlanCache = function() {
    return new PlanCache( this );
}

// Overrides connection-level settings.
//

DBCollection.prototype.setWriteConcern = function( wc ) {
    if ( wc instanceof WriteConcern ) {
        this._writeConcern = wc;
    }
    else {
        this._writeConcern = new WriteConcern( wc );
    }
};

DBCollection.prototype.getWriteConcern = function() {
    if (this._writeConcern)
        return this._writeConcern;


    if (this._db.getWriteConcern())
        return this._db.getWriteConcern();

    return null;
};

DBCollection.prototype.unsetWriteConcern = function() {
    delete this._writeConcern;
};

/**
 * PlanCache
 * Holds a reference to the collection.
 * Proxy for planCache* commands.
 */
if ( ( typeof  PlanCache ) == "undefined" ){
    PlanCache = function( collection ){
        this._collection = collection;
    }
}

/**
 * Name of PlanCache.
 * Same as collection.
 */
PlanCache.prototype.getName = function(){
    return this._collection.getName();
}


/**
 * toString prints the name of the collection
 */
PlanCache.prototype.toString = function(){
    return "PlanCache for collection " + this.getName() + '. Type help() for more info.';
}

PlanCache.prototype.shellPrint = PlanCache.prototype.toString;

/**
 * Displays help for a PlanCache object.
 */
PlanCache.prototype.help = function () {
    var shortName = this.getName();
    print("PlanCache help");
    print("\tdb." + shortName + ".getPlanCache().help() - show PlanCache help");
    print("\tdb." + shortName + ".getPlanCache().listQueryShapes() - " +
          "displays all query shapes in a collection");
    print("\tdb." + shortName + ".getPlanCache().clear() - " +
          "drops all cached queries in a collection");
    print("\tdb." + shortName + ".getPlanCache().clearPlansByQuery(query[, projection, sort]) - " +
          "drops query shape from plan cache");
    print("\tdb." + shortName + ".getPlanCache().getPlansByQuery(query[, projection, sort]) - " +
          "displays the cached plans for a query shape");
    return __magicNoPrint;
}

/**
 * Internal function to parse query shape.
 */
PlanCache.prototype._parseQueryShape = function(query, projection, sort) {
    if (query == undefined) {
        throw new Error("required parameter query missing");
    }

    // Accept query shape object as only argument.
    // Query shape contains exactly 3 fields (query, projection and sort)
    // as generated in the listQueryShapes() result.
    if (typeof(query) == 'object' && projection == undefined && sort == undefined) {
        var keysSorted = Object.keys(query).sort();
        // Expected keys must be sorted for the comparison to work.
        if (bsonWoCompare(keysSorted, ['projection', 'query', 'sort']) == 0) {
            return query;
        }
    }

    // Extract query shape, projection and sort from DBQuery if it is the first
    // argument. If a sort or projection is provided in addition to DBQuery, do not
    // overwrite with the DBQuery value.
    if (query instanceof DBQuery) {
        if (projection != undefined) {
            throw new Error("cannot pass DBQuery with projection");
        }
        if (sort != undefined) {
            throw new Error("cannot pass DBQuery with sort");
        }

        var queryObj = query._query["query"] || {}
        projection = query._fields || {};
        sort = query._query["orderby"] || {};
        // Overwrite DBQuery with the BSON query.
        query = queryObj;
    }

    var shape = {
        query: query,
        projection: projection == undefined ? {} : projection,
        sort: sort == undefined ? {} : sort,
    };
    return shape;
}

/**
 * Internal function to run command.
 */
PlanCache.prototype._runCommandThrowOnError = function(cmd, params) {
    var res = this._collection.runCommand(cmd, params);
    if (!res.ok) {
        throw new Error(res.errmsg);
    }
    return res;
}

/**
 * Lists query shapes in a collection.
 */
PlanCache.prototype.listQueryShapes = function() {
    return this._runCommandThrowOnError("planCacheListQueryShapes", {}).shapes;
}

/**
 * Clears plan cache in a collection.
 */
PlanCache.prototype.clear = function() {
    this._runCommandThrowOnError("planCacheClear", {});
    return;
}

/**
 * List plans for a query shape.
 */
PlanCache.prototype.getPlansByQuery = function(query, projection, sort) {
    return this._runCommandThrowOnError("planCacheListPlans",
                                        this._parseQueryShape(query, projection, sort)).plans;
}

/**
 * Drop query shape from the plan cache.
 */
PlanCache.prototype.clearPlansByQuery = function(query, projection, sort) {
    this._runCommandThrowOnError("planCacheClear", this._parseQueryShape(query, projection, sort));
    return;
}
