// Test cloning a database from a replica set (as full replica set uri, just the PRIMARY, or just a
// SECONDARY) to a standalone server and viceversa (SERVER-1643)

(function() {
    "use strict";

if (jsTest.options().keyFile) {
    jsTest.log("Skipping test because clone command doesn't work with authentication enabled:" +
              " SERVER-4245");
} else {
    var numDocs = 2000;

    // 1kb string
    var str = new Array(1000).toString();

    var replsetDBName = 'cloneDBreplset';
    var standaloneDBName = 'cloneDBstandalone';
    var testColName = 'foo';

    jsTest.log("Create replica set");
    var replTest = new ReplSetTest({name: 'testSet', nodes: 3});
    replTest.startSet();
    replTest.initiate();
    var master = replTest.getMaster();
    var secondary = replTest.liveNodes.slaves[0];
    var masterDB = master.getDB(replsetDBName);
    masterDB.dropDatabase();
    
    jsTest.log("Create standalone server");
    var standalone = MongoRunner.runMongod();
    standalone.getDB("admin").runCommand({setParameter:1,logLevel:5});
    var standaloneDB = standalone.getDB(replsetDBName);
    standaloneDB.dropDatabase();
    
    jsTest.log("Insert data into replica set");
    var bulk = masterDB[testColName].initializeUnorderedBulkOp();
    for (var i = 0; i < numDocs; i++) {
        bulk.insert({x: i, text: str});
    }
    assert.writeOK(bulk.execute());
    
    jsTest.log("Clone db from replica set to standalone server");
    standaloneDB.cloneDatabase(replTest.getURL());
    assert.eq(numDocs, standaloneDB[testColName].count(),
              'cloneDatabase from replset to standalone failed (document counts do not match)');

    jsTest.log("Clone db from replica set PRIMARY to standalone server");
    standaloneDB.dropDatabase();
    standaloneDB.cloneDatabase(master.host);
    assert.eq(numDocs, standaloneDB[testColName].count(),
              'cloneDatabase from PRIMARY to standalone failed (document counts do not match)');
    
    /* cloning from a SECONDARY does not work (SERVER-13357)
    jsTest.log("Clone db from replica set SECONDARY to standalone server");
    standaloneDB.dropDatabase();
    standaloneDB.cloneDatabase(secondary.host);
    assert.eq(numDocs, standaloneDB[testColName].count(),
              'cloneDatabase from SECONDARY to standalone failed (document counts do not match)');
    */
    
    jsTest.log("Switch db and insert data into standalone server");
    masterDB = master.getDB(standaloneDBName);
    var secondaryDB = secondary.getDB(standaloneDBName);
    standaloneDB = standalone.getDB(standaloneDBName);
    masterDB.dropDatabase();
    secondaryDB.dropDatabase();
    standaloneDB.dropDatabase();

    bulk = standaloneDB[testColName].initializeUnorderedBulkOp();
    for (var i = 0; i < numDocs; i++) {
        bulk.insert({x: i, text: str});
    }
    assert.writeOK(bulk.execute());

    jsTest.log("Clone db from standalone server to replica set PRIMARY");
    masterDB.cloneDatabase(standalone.host);
    replTest.awaitReplication();
    assert.eq(numDocs, masterDB[testColName].count(),
              'cloneDatabase from standalone to PRIMARY failed (document counts do not match)');

    jsTest.log("Clone db from standalone server to replica set SECONDARY");
    masterDB.dropDatabase();
    replTest.awaitReplication();
    secondaryDB.cloneDatabase(standalone.host);
    assert.eq(0, secondaryDB[testColName].count(),
              'cloneDatabase from standalone to SECONDARY succeeded and should not accept writes');

    jsTest.log("Shut down replica set and standalone server");
    MongoRunner.stopMongod(standalone.port);

    replTest.stopSet();
}

})();
