
(function() {
    var compare_configs = function(c1, c2) {
        assert.eq(c1.version, c2.version, 'version same');
        assert.eq(c1._id, c2._id, '_id same');

        for (var i in c1.members) {
            assert(c2.members[i] !== undefined, 'field '+i+' exists in both configs');
            assert.eq(c1.members[i]._id, c2.members[i]._id, 'id is equal in both configs');
            assert.eq(c1.members[i].host, c2.members[i].host, 'host is equal in both configs');
        }
    };

    // Make sure that we can restart a replica set completely
    // Also, ensure config is saved properly between restarts.

    // Create a new replica set test. Specify set name and the number of nodes you want.
    var replTest = new ReplSetTest( {name: 'testSet', nodes: 3} );

    // call startSet() to start each mongod in the replica set
    // this returns a list of nodes
    replTest.startSet();

    // Call initiate() to send the replSetInitiate command
    // This will wait for initiation
    replTest.initiate();

    // Wait for at least one heartbeat to reach everyone, so that we will properly mark nodes as
    // DOWN, later.
    replTest.awaitSecondaryNodes();

    // Call getMaster to return a reference to the node that's been
    // elected master.
    var master = replTest.getMaster();
    var config1 = master.getDB("local").system.replset.findOne();

    // Now we're going to shut down all nodes
    var mId  = replTest.getNodeId( master );
    var s1 = replTest.liveNodes.slaves[0];
    var s1Id = replTest.getNodeId(s1);
    var s2 = replTest.liveNodes.slaves[1];
    var s2Id = replTest.getNodeId(s2);

    replTest.stop( s1Id );
    replTest.stop( s2Id );
    replTest.waitForState(s1, replTest.DOWN);
    replTest.waitForState(s2, replTest.DOWN);
    
    replTest.stop( mId );

    // Now let's restart these nodes
    replTest.restart( mId );
    replTest.restart( s1Id );
    replTest.restart( s2Id );

    // Make sure that a new master comes up
    master = replTest.getMaster();
    replTest.awaitSecondaryNodes();
    var config2 = master.getDB("local").system.replset.findOne();
    compare_configs(config1, config2);
}());
