// This test is to ensure that limit() clauses are pushed down to the shards and evaluated
// See: http://jira.mongodb.org/browse/SERVER-1896

s = new ShardingTest( "limit_push", 2, 1, 1 );

// Stop balancer since we do manual moves.
s.stopBalancer();

db = s.getDB( "test" );

// Create some data
for (i=0; i < 100; i++) { db.limit_push.insert({ _id : i, x: i}); } 
db.limit_push.ensureIndex( { x : 1 } );
assert.eq( 100 , db.limit_push.find().length() , "Incorrect number of documents" );

// Shard the collection
s.adminCommand( { enablesharding : "test" } );
s.adminCommand( { shardcollection : "test.limit_push" , key : { x : 1 } } );

// Now split the and move the data between the shards
s.adminCommand( { split : "test.limit_push", middle : { x : 50 }} ); 
s.adminCommand( { moveChunk: "test.limit_push", find : { x : 51}, to : "shard0000", _waitForDelete : true }) 

// Check that the chunck have split correctly
assert.eq( 2 , s.config.chunks.count() , "wrong number of chunks");

// The query is asking for the maximum value below a given value
// db.limit_push.find( { x : { $lt : 60} } ).sort( { x:-1} ).limit(1)
q = { x : { $lt : 60} }; 

// Make sure the basic queries are correct
assert.eq( 60 , db.limit_push.find( q ).count() , "Did not find 60 documents" );
//rs = db.limit_push.find( q ).sort( { x:-1} ).limit(1)
//assert.eq( rs , { _id : "1" , x : 59 } , "Did not find document with value 59" );

// Now make sure that the explain shos that each shard is returning a single document as indicated
// by the "n" element for each shard
exp = db.limit_push.find( q ).sort( { x:-1} ).limit(1).explain();
printjson( exp )

assert.eq("ParallelSort", exp.clusteredType, "Not a ParallelSort");

var k = 0;
for (var j in exp.shards) {
    assert.eq( 1 , exp.shards[j][0].n, "'n' is not 1 from shard000" + k.toString());
    k++
}

s.stop();

