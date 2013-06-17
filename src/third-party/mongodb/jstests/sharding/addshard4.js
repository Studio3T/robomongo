// a replica set's passive nodes should be okay to add as part of a shard config

s = new ShardingTest( "addshard4", 2 , 0 , 1 , {useHostname : true});

r = new ReplSetTest({name : "addshard4", nodes : 3, startPort : 31100});
r.startSet();

var config = r.getReplSetConfig();
config.members[2].priority = 0;

r.initiate(config);
//Wait for replica set to be fully initialized - could take some time
//to pre-allocate files on slow systems
r.awaitReplication();

var master = r.getMaster();

var members = config.members.map(function(elem) { return elem.host; });
var shardName = "addshard4/"+members.join(",");
var invalidShardName = "addshard4/foobar";

print("adding shard "+shardName);

// First try adding shard with the correct replica set name but incorrect hostname
// This will make sure that the metadata for this replica set name is cleaned up
// so that the set can be added correctly when it has the proper hostnames.
assert.throws(function() {s.adminCommand({"addshard" : invalidShardName});});

var result = s.adminCommand({"addshard" : shardName});

printjson(result);
assert.eq(result, true);

r = new ReplSetTest({name : "addshard42", nodes : 3, startPort : 31200});
r.startSet();

config = r.getReplSetConfig();
config.members[2].arbiterOnly = true;

r.initiate(config);
// Wait for replica set to be fully initialized - could take some time
// to pre-allocate files on slow systems
r.awaitReplication();

master = r.getMaster();

print("adding shard addshard42");

result = s.adminCommand({"addshard" : "addshard42/"+config.members[2].host});

printjson(result);
assert.eq(result, true);
