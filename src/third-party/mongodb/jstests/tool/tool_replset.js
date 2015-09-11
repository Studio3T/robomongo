/*
 * Test to ensure that (dump/restore/export/import/oplog) works with a replica set connection string
 * 1. Start a replica set.
 * 2. Add data to a collection.
 * 3. Take a dump of the database.
 * 4. Drop the db.
 * 5. Restore the db.
 * 6. Export a collection.
 * 7. Drop the collection.
 * 8. Import the collection.
 * 9. Add data to the oplog.rs collection.
 * 10. Ensure that the document doesn't exist yet.
 * 11. Now play the mongooplog tool.
 * 12. Make sure that the oplog was played
*/

(function() {
    "use strict";

    var replTest = new ReplSetTest({ name: 'tool_replset', nodes: 2, oplogSize: 5 });
    var nodes = replTest.startSet();
    var config = replTest.getReplSetConfig();
    config.members[0].priority = 3;
    config.members[1].priority = 0;
    replTest.initiate(config);
    var master = replTest.getMaster();
    assert.eq(nodes[0], master, "incorrect master elected");
    for (var i = 0; i < 100; i++) {
        assert.writeOK(master.getDB("foo").bar.insert({ a: i }));
    }
    replTest.awaitReplication();

    var replSetConnString = "tool_replset/127.0.0.1:" + replTest.ports[0] +
                            ",127.0.0.1:" + replTest.ports[1];

    // Test with mongodump/mongorestore
    print("dump the db");
    var data = MongoRunner.dataDir + "/tool_replset-dump1/";
    runMongoProgram("mongodump", "--host", replSetConnString, "--out", data);

    print("db successfully dumped, dropping now");
    master.getDB("foo").dropDatabase();
    replTest.awaitReplication();

    print("restore the db");
    runMongoProgram("mongorestore", "--host", replSetConnString, "--dir", data);

    print("db successfully restored, checking count")
    var x = master.getDB("foo").getCollection("bar").count();
    assert.eq(x, 100, "mongorestore should have successfully restored the collection");

    replTest.awaitReplication();

    // Test with mongoexport/mongoimport
    print("export the collection");
    var extFile = MongoRunner.dataDir + "/tool_replset/export";
    runMongoProgram("mongoexport", "--host", replSetConnString, "--out", extFile,
                    "-d", "foo", "-c", "bar");

    print("collection successfully exported, dropping now");
    master.getDB("foo").getCollection("bar").drop();
    replTest.awaitReplication();

    print("import the collection");
    runMongoProgram("mongoimport", "--host", replSetConnString, "--file", extFile,
                    "-d", "foo", "-c", "bar");

    var x = master.getDB("foo").getCollection("bar").count();
    assert.eq(x, 100, "mongoimport should have successfully imported the collection");
    var doc = {_id: 5, x: 17};
    var oplogEntry = {ts: new Timestamp(), "op": "i", "ns": "foo.bar", "o": doc, "v": NumberInt(2)}
    assert.writeOK(master.getDB("local").oplog.rs.insert(oplogEntry));

    assert.eq(100, master.getDB("foo").getCollection("bar").count(), "count before running " +
              "mongooplog was not 100 as expected");

    runMongoProgram("mongooplog" , "--from", "127.0.0.1:" + replTest.ports[0],
                                   "--host", replSetConnString);

    print("finished running mongooplog to replay the oplog")

    assert.eq(101, master.getDB("foo").getCollection("bar").count(), "count after running " +
              "mongooplog was not 101 as expected")

    print("all tests successful, stopping replica set")

    replTest.stopSet();

    print("replica set stopped, test complete")
}());
