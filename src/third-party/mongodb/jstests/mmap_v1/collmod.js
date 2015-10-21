// Basic js tests for the collMod command.
// Test setting the usePowerOf2Sizes flag, and modifying TTL indexes.

function debug( x ) {
    //printjson( x );
}

var coll = "collModTest";
var t = db.getCollection( coll );
t.drop();

db.createCollection( coll );


// Verify the new collection has userFlags set to 1
printjson(t.stats());
assert.eq( t.stats().userFlags , 1 , "fresh collection doesn't have userFlags = 1 ");

// Modify the collection with the usePowerOf2Sizes flag. Verify userFlags now = 0.
var res = db.runCommand( { "collMod" : coll,  "usePowerOf2Sizes" : false } );
debug( res );
assert.eq( res.ok , 1 , "collMod failed" );
assert.eq( t.stats().userFlags , 0 , "modified collection should have userFlags = 0 ");
var nso = db.system.namespaces.findOne( { name : t.getFullName() } );
debug( nso );
assert.eq( 0, nso.options.flags, "options didn't sync to system.namespaces: " + tojson( nso ) );

// Try to modify it with some unrecognized value
var res = db.runCommand( { "collMod" : coll,  "unrecognized" : true } );
debug( res );
assert.eq( res.ok , 0 , "collMod shouldn't return ok with unrecognized value" );

// add a TTL index
t.ensureIndex( {a : 1}, { "expireAfterSeconds": 50 } )
assert.eq( 1, db.system.indexes.count( { key : {a:1}, expireAfterSeconds : 50 } ),
           "TTL index not added" );

// try to modify it with a bad key pattern
var res = db.runCommand( { "collMod" : coll,
                           "index" : { "keyPattern" : "bad" , "expireAfterSeconds" : 100 } } );
debug( res );
assert.eq( 0 , res.ok , "mod shouldn't work with bad keypattern");

// try to modify it without expireAfterSeconds field
var res = db.runCommand( { "collMod" : coll,
                         "index" : { "keyPattern" : {a : 1} } } );
debug( res );
assert.eq( 0 , res.ok , "TTL mod shouldn't work without expireAfterSeconds");

// try to modify it with a non-numeric expireAfterSeconds field
var res = db.runCommand( { "collMod" : coll,
                           "index" : { "keyPattern" : {a : 1}, "expireAfterSeconds" : "100" } } );
debug( res );
assert.eq( 0 , res.ok , "TTL mod shouldn't work with non-numeric expireAfterSeconds");

// this time modifying should finally  work
var res = db.runCommand( { "collMod" : coll,
                           "index" : { "keyPattern" : {a : 1}, "expireAfterSeconds" : 100 } } );
debug( res );
assert.eq( 1, db.system.indexes.count( { key : {a:1}, expireAfterSeconds : 100 } ),
           "TTL index not modified" );

// try to modify a faulty TTL index with a non-numeric expireAfterSeconds field
t.dropIndex( {a : 1 } );
t.ensureIndex( {a : 1} , { "expireAfterSeconds": "50" } )
var res = db.runCommand( { "collMod" : coll,
                           "index" : { "keyPattern" : {a : 1} , "expireAfterSeconds" : 100 } } );
debug( res );
assert.eq( 0, res.ok, "shouldn't be able to modify faulty index spec" );

// try with new index, this time set both expireAfterSeconds and the usePowerOf2Sizes flag
t.dropIndex( {a : 1 } );
t.ensureIndex( {a : 1} , { "expireAfterSeconds": 50 } )
var res = db.runCommand( { "collMod" : coll ,
                           "usePowerOf2Sizes" : true,
                           "index" : { "keyPattern" : {a : 1} , "expireAfterSeconds" : 100 } } );
debug( res );
assert.eq( 1, res.ok, "should be able to modify both userFlags and expireAfterSeconds" );
assert.eq( t.stats().userFlags , 1 , "userflags should be 1 now");
assert.eq( 1, db.system.indexes.count( { key : {a:1}, expireAfterSeconds : 100 } ),
           "TTL index should be 100 now" );

