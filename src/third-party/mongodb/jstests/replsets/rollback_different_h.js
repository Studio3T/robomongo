// test that a rollback of an entry that matches the ts but not h causes no problems

// function to check the logs for an entry
doesEntryMatch = function(array, regex) {
    var found = false;
    for (i = 0; i < array.length; i++) {
        if (regex.test(array[i])) {
            found = true;
        }
    }
    return found;
}

// set up a set and grab things for later
var name = "rollback_different_h";
var replTest = new ReplSetTest({name: name, nodes: 3});
var nodes = replTest.nodeList();
var conns = replTest.startSet();
replTest.initiate({"_id": name,
                   "members": [
                       { "_id": 0, "host": nodes[0], priority: 3 },
                       { "_id": 1, "host": nodes[1] },
                       { "_id": 2, "host": nodes[2], arbiterOnly: true}]
                  });
var a_conn = conns[0];
var b_conn = conns[1];
var AID = replTest.getNodeId(a_conn);
var BID = replTest.getNodeId(b_conn);

replTest.waitForState(replTest.nodes[0], replTest.PRIMARY, 60 * 1000);

// get master and do an initial write
var master = replTest.getMaster();
assert(master === conns[0], "conns[0] assumed to be master");
assert(a_conn.host === master.host, "a_conn assumed to be master");
var options = {writeConcern: {w: 2, wtimeout: 60000}, upsert: true};
assert.writeOK(a_conn.getDB(name).foo.insert({x: 1}, options));

// shut down master
replTest.stop(AID);

// change the h value of the most recent entry on B
master = replTest.getMaster();
assert(b_conn.host === master.host, "b_conn assumed to be master");
options = {writeConcern: {w: 1, wtimeout: 60000}, upsert: true};
var oplog_entry = b_conn.getDB("local").oplog.rs.find().sort({$natural: -1})[0];
oplog_entry["ts"].t++;
oplog_entry["h"] = NumberLong(1);
res = b_conn.getDB("local").oplog.rs.insert(oplog_entry);
assert( res.nInserted > 0, tojson( res ) );

// another insert to set minvalid ahead
assert.writeOK(b_conn.getDB(name).foo.insert({x: 123}));

// shut down B and bring back the original master
replTest.stop(BID);
replTest.restart(AID);
master = replTest.getMaster();
assert(a_conn.host === master.host, "a_conn assumed to be master");

// do a write so that B will have to roll back
options = {writeConcern: {w: 1, wtimeout: 60000}, upsert: true};
assert.writeOK(a_conn.getDB(name).foo.insert({x: 2}, options));

// restart B, which should rollback and get to the same state as A
replTest.restart(BID);
assert.soon(function() {
    try {
        var logb = b_conn.getDB("local").oplog.rs.find().toArray();
        var loga = a_conn.getDB("local").oplog.rs.find().toArray();
        for (var i = 0; i < loga.length; i++) {
            if (tojson(loga[i]) !== tojson(logb[i])) {
                return false;
            }
        }
        return true;
    }
    catch (e) {
        return false;
    }
}, "oplogs from A and B did not match after rollback");

replTest.stopSet();
