// SERVER-13702
// Some commands allow optional query, e.g. count, mapreduce.
// If the optional query is not given, mongos will wrongly use the command
// BSONObj itself as the query to target shards, which could return wrong
// shards if the shard key happens to be one of the fields in the command object.

var s = new ShardingTest("shard_targeting", 2, 0, 1);
s.adminCommand({ enablesharding : "test" });
s.stopBalancer();

var db = s.getDB("test");
var res;

//
// Target count command
//

// Shard key is the same with command name.
s.shardGo("foo", {count: 1}, { count: "" })

for (var i=0; i<50; i++) {
  db.foo.insert({count: i}); // chunk [MinKey, ""), including numbers
  db.foo.insert({count: "" + i}); // chunk ["", MaxKey]
}

var theOtherShard = s.getOther( s.getServer( "test" ) ).name;
printShardingStatus();

// Count documents on both shards

// "count" commnad with "query" option { }.
assert.eq(db.foo.count(), 100);
// Optional "query" option is not given.
res = db.foo.runCommand("count");
assert.eq(res.n, 100);

//
// Target mapreduce command
//
db.foo.drop();
// Shard key is the same with command name.
s.shardGo("foo", {mapReduce: 1}, { mapReduce: "" })

for (var i=0; i<50; i++) {
  db.foo.insert({mapReduce: i}); // to the chunk including number
  db.foo.insert({mapReduce: "" + i}); // to the chunk including string
}

printShardingStatus();

function m() { emit("total", 1); }
function r(k, v) { return Array.sum(v); }
res = db.foo.runCommand(
{
             mapReduce: "foo",
             map: m,
             reduce:  r,
             out: { inline: 1 }
});

// Count documents on both shards
assert.eq(res.results[0].value, 100);

s.stop();
