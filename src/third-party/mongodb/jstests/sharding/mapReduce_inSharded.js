var verifyOutput = function(out) {
    printjson(out);
    assert.eq(out.counts.input, 51200, "input count is wrong");
    assert.eq(out.counts.emit, 51200, "emit count is wrong");
    assert.gt(out.counts.reduce, 99, "reduce count is wrong");
    assert.eq(out.counts.output, 512, "output count is wrong");
}

var st = new ShardingTest({ shards : 2, verbose : 1, mongos : 1, other : { chunksize : 1 } });
st.startBalancer();

st.adminCommand( { enablesharding : "mrShard" } )
st.adminCommand( { shardcollection : "mrShard.srcSharded", key : { "_id" : 1 } } )

var db = st.getDB( "mrShard" );

var bulk = db.srcSharded.initializeUnorderedBulkOp();
for (j = 0; j < 100; j++) {
    for (i = 0; i < 512; i++) {
        bulk.insert({ j: j, i: i });
    }
}
assert.writeOK(bulk.execute());

function map() { emit(this.i, 1); }
function reduce(key, values) { return Array.sum(values) } 

// sharded src
var suffix = "InSharded";

var out = db.srcSharded.mapReduce(map, reduce, "mrBasic" + suffix);
verifyOutput(out);

out = db.srcSharded.mapReduce(map, reduce, { out: { replace: "mrReplace" + suffix } });
verifyOutput(out);

out = db.srcSharded.mapReduce(map, reduce, { out: { merge: "mrMerge" + suffix } });
verifyOutput(out);

out = db.srcSharded.mapReduce(map, reduce, { out: { reduce: "mrReduce" + suffix } });
verifyOutput(out);

out = db.srcSharded.mapReduce(map, reduce, { out: { inline: 1 } });
verifyOutput(out);
assert(out.results != 'undefined', "no results for inline");

out = db.srcSharded.mapReduce(map, reduce, { out: { replace: "mrReplace" + suffix, db: "mrShardOtherDB" } });
verifyOutput(out);

out = db.runCommand({
    mapReduce: "srcSharded", // use new name mapReduce rather than mapreduce
    map: map,
    reduce: reduce,
    out: "mrBasic" + "srcSharded",
  });
verifyOutput(out);
