
// Test db.loadServerScripts()

var testdb = db.getSisterDB("loadserverscripts");

jsTest.log("testing db.loadServerScripts()");
var x;

// assert._debug = true;

// clear out any data from old tests
testdb.system.js.remove({});
delete myfunc;
delete myfunc2;

x = testdb.system.js.findOne();
assert.isnull(x, "Test for empty collection");

// User functions should not be defined yet
assert.eq( typeof myfunc, "undefined", "Checking that myfunc() is undefined" );
assert.eq( typeof myfunc2, "undefined", "Checking that myfunc2() is undefined" );

// Insert a function in the context of this process: make sure it's in the collection
testdb.system.js.insert( { _id: "myfunc", "value": function(){ return "myfunc"; } } );
x = testdb.system.js.count();
assert.eq( x, 1, "Should now be one function in the system.js collection");

// Load that function
testdb.loadServerScripts();
assert.eq( typeof myfunc, "function", "Checking that myfunc() loaded correctly" );

// Make sure it works
x = myfunc();
assert.eq(x, "myfunc", "Checking that myfunc() returns the correct value");

// Insert value into collection from another process
var coproc = startParallelShell(
        'db.getSisterDB("loadserverscripts").system.js.insert' +
        '    ( {_id: "myfunc2", "value": function(){ return "myfunc2"; } } );'
                );
// wait for results
coproc();

// Make sure the collection's been updated
x = testdb.system.js.count();
assert.eq( x, 2, "Should now be two functions in the system.js collection");


// Load the new functions: test them as above
testdb.loadServerScripts();
assert.eq( typeof myfunc2, "function", "Checking that myfunc2() loaded correctly" );
x = myfunc2();
assert.eq(x, "myfunc2", "Checking that myfunc2() returns the correct value");

jsTest.log("completed test of db.loadServerScripts()");

