var wait, occasionally, reconnect, getLatestOp, waitForAllMembers, reconfig, awaitOpTime;
(function () {
"use strict";
var count = 0;
var w = 0;

wait = function(f,msg) {
    w++;
    var n = 0;
    while (!f()) {
        if( n % 4 == 0 )
            print("waiting " + w);
        if (++n == 4) {
            print("" + f);
        }
        assert(n < 200, 'tried 200 times, giving up on ' + msg );
        sleep(1000);
    }
};

/**
 * Use this to do something once every 4 iterations.
 *
 * <pre>
 * for (i=0; i<1000; i++) {
 *   occasionally(function() { print("4 more iterations"); });
 * }
 * </pre>
 */
occasionally = function(f, n) {
  var interval = n || 4;
  if (count % interval == 0) {
    f();
  }
  count++;
};

reconnect = function(a) {
  wait(function() {
      var db;
      try {
        // make this work with either dbs or connections
        if (typeof(a.getDB) == "function") {
          db = a.getDB('foo');
        }
        else {
          db = a;
        }
        db.bar.stats();
        if (jsTest.options().keyFile || jsTest.options().useX509) { // SERVER-4241: Shell connections don't re-authenticate on reconnect
          return jsTest.authenticate(db.getMongo());
        }
        return true;
      } catch(e) {
        print(e);
        return false;
      }
    });
};


getLatestOp = function(server) {
    server.getDB("admin").getMongo().setSlaveOk();
    var log = server.getDB("local")['oplog.rs'];
    var cursor = log.find({}).sort({'$natural': -1}).limit(1);
    if (cursor.hasNext()) {
      return cursor.next();
    }
    return null;
};


waitForAllMembers = function(master, timeout) {
    var failCount = 0;

    assert.soon( function() {
        var state = null
        try {
            state = master.getSisterDB("admin").runCommand({replSetGetStatus:1});
            failCount = 0;
        } catch ( e ) {
            // Connection can get reset on replica set failover causing a socket exception
            print( "Calling replSetGetStatus failed" );
            print( e );
            return false;
        }
        occasionally(function() { printjson(state); }, 10);

        for (var m in state.members) {
            if (state.members[m].state != 1 && // PRIMARY
                state.members[m].state != 2 && // SECONDARY
                state.members[m].state != 7) { // ARBITER
                return false;
            }
        }
        printjson( state );
        return true;
    }, "not all members ready", timeout || 60000);

    print( "All members are now in state PRIMARY, SECONDARY, or ARBITER" );
};

reconfig = function(rs, config, force) {
    "use strict";
    var admin = rs.getMaster().getDB("admin");
    var e;
    var master;
    try {
        assert.commandWorked(admin.runCommand({replSetReconfig: config, force: force}));
    }
    catch (e) {
        if (tojson(e).indexOf( "error doing query: failed" ) < 0) {
            throw e;
        }
    }

    var master = rs.getMaster().getDB("admin");
    waitForAllMembers(master);

    return master;
};

awaitOpTime = function (node, opTime) {
    var ts, ex;
    assert.soon(function () {
        try {
            // The following statement extracts the timestamp field from the most recent element of
            // the oplog, and stores it in "ts".
            ts = node.getDB("local")['oplog.rs'].find({}).sort({'$natural': -1}).limit(1).next().ts;
            if ((ts.t == opTime.t) && (ts.i == opTime.i)) {
                return true;
            }
            ex = null;
            return false;
        }
        catch (ex) {
            return false;
        }
    }, function () {
        var message = "Node " + node + " only reached optime " + tojson(ts) + " not " +
            tojson(opTime);
        if (ex) {
            message += "; last attempt failed with exception " + tojson(ex);
        }
        return message;
    });
};

}());
