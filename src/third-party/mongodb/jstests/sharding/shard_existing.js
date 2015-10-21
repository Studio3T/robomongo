s = new ShardingTest( "shard_existing" , 2 /* numShards */, 1 /* verboseLevel */, 1 /* numMongos */, { chunksize : 1 } )

db = s.getDB( "test" )

var stringSize = 10000;
var numDocs = 2000;


// we want a lot of data, so lets make a string to cheat :)
var bigString = new Array(stringSize).toString();
var docSize = Object.bsonsize({ _id: numDocs, s: bigString });
var totalSize = docSize * numDocs;
print("NumDocs: " + numDocs + " DocSize: " + docSize + " TotalSize: " + totalSize);

var bulk = db.data.initializeUnorderedBulkOp();
for (i=0; i<numDocs; i++) {
    bulk.insert({_id: i, s: bigString});
}
assert.writeOK(bulk.execute());

var avgObjSize = db.data.stats().avgObjSize;
var dataSize = db.data.stats().size;
assert.lte(totalSize, dataSize);

s.adminCommand( { enablesharding : "test" } );
res = s.adminCommand( { shardcollection : "test.data" , key : { _id : 1 } } );
printjson(res);

// number of chunks should be approx equal to the total data size / half the chunk size
var numChunks = s.config.chunks.find().itcount();
var guess = Math.ceil(dataSize / (512*1024 + avgObjSize));
assert( Math.abs( numChunks - guess ) < 2, "not right number of chunks" );

s.stop();
