//
// Tests the state of sharding data after a replica set reconfig
//

var options = { separateConfig : true,
                rs : true,
                rsOptions : { nodes : 1 } };

var st = new ShardingTest({ shards : 2, mongos : 1, other : options });
st.stopBalancer();

var mongos = st.s0;
var admin = mongos.getDB("admin");
var shards = mongos.getCollection("config.shards").find().toArray();

var coll = mongos.getCollection("foo.bar");
var collSharded = mongos.getCollection("foo.barSharded");

assert.commandWorked(admin.runCommand({ enableSharding : coll.getDB() + "" }));
printjson(admin.runCommand({ movePrimary : coll.getDB() + "", to : shards[0]._id }));
assert.commandWorked(admin.runCommand({ shardCollection : collSharded.toString(),
                                        key : { _id : 1 } }));
assert.commandWorked(admin.runCommand({ moveChunk : collSharded.toString(),
                                        find : { _id : 0 },
                                        to : shards[1]._id }));

assert.writeOK(coll.insert({ some : "data" }));
assert.writeOK(collSharded.insert({ some : "data" }));
assert.eq(2, mongos.adminCommand({ getShardVersion : collSharded.toString() }).version.t);

st.printShardingStatus();

// Restart both primaries to reset our sharding data
var restartPrimaries = function() {

    var rs0Primary = st.rs0.getPrimary();
    var rs1Primary = st.rs1.getPrimary();

    st.rs0.stop(rs0Primary);
    st.rs1.stop(rs1Primary);

    ReplSetTest.awaitRSClientHosts(mongos, [rs0Primary, rs1Primary], { ok : false });

    st.rs0.start(rs0Primary, { restart : true });
    st.rs1.start(rs1Primary, { restart : true });

    ReplSetTest.awaitRSClientHosts(mongos, [rs0Primary, rs1Primary], { ismaster : true });
};

restartPrimaries();

//
//
// No sharding data until shards are hit by a query
assert.eq("",
          st.rs0.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);
assert.eq("",
          st.rs1.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);

//
//
// Sharding data initialized when shards are hit by an unsharded query
assert.neq(null, coll.findOne({}));
assert.neq("",
          st.rs0.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);
assert.eq("",
          st.rs1.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);

//
//
// Sharding data initialized when shards are hit by a sharded query
assert.neq(null, collSharded.findOne({}));
assert.neq("",
           st.rs0.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);
assert.neq("",
           st.rs1.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);


// Stepdown both primaries to reset our sharding data
var stepDownPrimaries = function() {

    var rs0Primary = st.rs0.getPrimary();
    var rs1Primary = st.rs1.getPrimary();

    try {
        rs0Primary.adminCommand({ replSetStepDown : 1000 * 1000, force : true });
        assert(false);
    }
    catch(ex) {
        // Expected connection exception, will check for stepdown later
    }

    try {
        rs1Primary.adminCommand({ replSetStepDown : 1000 * 1000, force : true });
        assert(false);
    }
    catch(ex) {
        // Expected connection exception, will check for stepdown later
    }

    ReplSetTest.awaitRSClientHosts(mongos, [rs0Primary, rs1Primary], { secondary : true });

    assert.commandWorked(new Mongo(rs0Primary.host).adminCommand({ replSetFreeze : 0 }));
    assert.commandWorked(new Mongo(rs1Primary.host).adminCommand({ replSetFreeze : 0 }));

    rs0Primary = st.rs0.getPrimary();
    rs1Primary = st.rs1.getPrimary();

    // Flush connections to avoid transient issues with conn pooling
    assert.commandWorked(rs0Primary.adminCommand({ connPoolSync : true }));
    assert.commandWorked(rs1Primary.adminCommand({ connPoolSync : true }));

    ReplSetTest.awaitRSClientHosts(mongos, [rs0Primary, rs1Primary], { ismaster : true });
};

stepDownPrimaries();

//
//
// No sharding data until shards are hit by a metadata operation
assert.eq("",
          st.rs0.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);
assert.eq("",
          st.rs1.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);

//
//
// Metadata commands should enable sharding data implicitly
assert.commandWorked(mongos.adminCommand({ split : collSharded.toString(), middle : { _id : 0 }}));
assert.eq("",
           st.rs0.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);
assert.neq("",
          st.rs1.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);

//
//
// MoveChunk command should enable sharding data implicitly on TO-shard
assert.commandWorked(mongos.adminCommand({ moveChunk : collSharded.toString(), find : { _id : 0 },
                                           to : shards[0]._id }));
assert.neq("",
           st.rs0.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);
assert.neq("",
           st.rs1.getPrimary().adminCommand({ getShardVersion : coll.toString() }).configServer);

jsTest.log( "DONE!" );

st.stop();
