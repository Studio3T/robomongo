// SERVER-16498 d_migrate.cpp should not rely on system.namespaces
//
// This test creates a sharded collection with wiredtiger options.
// When the chunks of this collection get migrated to the other shard,
// the other shard should create the collection with the same options.
// However, before SERVER-16498, the receiver relies on checking
// system.namespaces on the donor, which is empty on wiredtiger.
// As a result, the new collection created on receiver has different
// options.
//
// P.S. wiredtiger options are not valid for MMAPv1, but MMAPv1 will
// keep and ignore them.

var st = new ShardingTest({ shards : 2 });

var db = st.s.getDB("test");
var coll = db.sharding_system_namespaces;

// This test relies on the wiredTiger storage engine being compiled
// into the server. This assumption does not hold on 32-bit platforms.
// See SERVER-16660.
if (db.serverBuildInfo().bits != 32) {

    function checkCollectionOptions(database) {
      var collectionsInfos = database.getCollectionInfos();
      printjson(collectionsInfos);
      var info = collectionsInfos.filter(function(c) {
        return c.name == "sharding_system_namespaces";
      })[0];
      assert.eq(info.options.storageEngine.wiredTiger.configString, "block_compressor=zlib");
    }

    db.createCollection("sharding_system_namespaces",
    {
      storageEngine: {
        wiredTiger: { configString: "block_compressor=zlib" }
      }
    });

    checkCollectionOptions(db);

    assert.commandWorked(db.adminCommand({ enableSharding: 'test' }));
    assert.commandWorked(db.adminCommand({ shardCollection: coll + '', key: { x: 1 }}));

    coll.insert({x: 0});
    coll.insert({x: 10});

    assert.commandWorked(db.adminCommand({ split: coll + '', middle: { x: 5 }}));

    printShardingStatus();

    var primaryShard = st.getServer("test");
    anotherShard = st.getOther( primaryShard );
    assert.commandWorked(db.adminCommand({
        movechunk: coll + '',
        find: { x: 5 },
        to: anotherShard.name
    }));

    printShardingStatus();

    checkCollectionOptions(anotherShard.getDB("test"));
}
