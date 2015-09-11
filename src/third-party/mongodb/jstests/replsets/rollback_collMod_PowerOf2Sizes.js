// test that a rollback of collModding usePowerOf2Sizes will cause the node to log a message about
// ignoring that action during rollback

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
var name = "rollback_collMod_PowerOf2Sizes";
var replTest = new ReplSetTest({name: name, nodes: 3});
var nodes = replTest.nodeList();
var conns = replTest.startSet();
replTest.initiate({"_id": name,
                   "members": [
                       { "_id": 0, "host": nodes[0] },
                       { "_id": 1, "host": nodes[1] },
                       { "_id": 2, "host": nodes[2], arbiterOnly: true}]
                  });
// get master and do an initial write
var master = replTest.getMaster();
var a_conn = master;
var slaves = replTest.liveNodes.slaves;
var b_conn = slaves[0];
var AID = replTest.getNodeId(a_conn);
var BID = replTest.getNodeId(b_conn);

var options = {writeConcern: {w: 2, wtimeout: 60000}, upsert: true};
assert.writeOK(a_conn.getDB(name).foo.insert({x: 1}, options));
assert.writeOK(a_conn.getDB(name).bar.insert({x: 1}, options));

// shut down master
replTest.stop(AID);

// collMod usePowerOf2Sizes to be false
master = replTest.getMaster();
assert(b_conn.host === master.host, "b_conn assumed to be master");
assert.commandWorked(b_conn.getDB(name).runCommand({collMod: "foo", usePowerOf2Sizes: false}));
assert.commandWorked(b_conn.getDB(name).runCommand({collMod: "bar", noPadding: true}));
assert.commandWorked(b_conn.getDB(name).runCommand({collMod: "bar", usePowerOf2Sizes: false, noPadding: true}));

// shut down B and bring back the original master
replTest.stop(BID);
replTest.restart(AID);
master = replTest.getMaster();
assert(a_conn.host === master.host, "a_conn assumed to be master");

// do a write so that B will have to roll back
options = {writeConcern: {w: 1, wtimeout: 60000}, upsert: true};
assert.writeOK(a_conn.getDB(name).foo.insert({x: 2}, options));

// restart B, which should rollback and log a message about not rolling back usePowerOf2Sizes
replTest.restart(BID);
var rollbackMsg = RegExp("replSet not rolling back change of usePowerOf2Sizes: ");
assert.soon(function() {
    try {
        var log = b_conn.getDB("admin").adminCommand({getLog: "global"}).log;
        return doesEntryMatch(log, rollbackMsg);
    }
    catch (e) {
        return false;
    }
}, "Did not see a log entry about skipping the usePowerOf2Sizes command during rollback");
var paddingMsg = RegExp("replSet not rolling back change of noPadding: ");
assert.soon(function() {
    try {
        var log = b_conn.getDB("admin").adminCommand({getLog: "global"}).log;
        return doesEntryMatch(log, paddingMsg);
    }
    catch (e) {
        return false;
    }
}, "Did not see a log entry about skipping the noPadding command during rollback");

replTest.stopSet();
