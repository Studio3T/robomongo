/**
 * Verify that killing an instance of mongod while it is in a long running computation or infinite
 * loop still leads to clean shutdown, and that said shutdown is prompt.
 *
 * For our purposes, "prompt" is defined as "before stopMongod() decides to send a SIGKILL", which
 * would not result in a zero return code.
 */

var port = allocatePorts( 1 )[ 0 ]

var baseName = "jstests_disk_killall";
var dbpath = MongoRunner.dataPath + baseName;

var mongod = startMongod( "--port", port, "--dbpath", dbpath, "--nohttpinterface" );
var db = mongod.getDB( "test" );
var collection = db.getCollection( baseName );
assert.writeOK(collection.insert({}));

var s1 = startParallelShell(
            "db." + baseName + ".count( { $where: function() { while( 1 ) { ; } } } )",
            port);
sleep( 1000 );

/**
 * 0 == mongod's exit code on Windows, or when it receives TERM, HUP or INT signals.  On UNIX
 * variants, stopMongod sends a TERM signal to mongod, then waits for mongod to stop.  If mongod
 * doesn't stop in a reasonable amount of time, stopMongod sends a KILL signal, in which case mongod
 * will not exit cleanly.  We're checking in this assert that mongod will stop quickly even while
 * evaling an infinite loop in server side js.
 */
var exitCode = stopMongod( port );
assert.eq(0, exitCode, "got unexpected exitCode");

// Waits for shell to complete
s1();

mongod = startMongoProgram( "mongod", "--port", port, "--dbpath", dbpath );
db = mongod.getDB( "test" );
collection = db.getCollection( baseName );

assert( collection.stats().ok );
assert( collection.drop() );

stopMongod( port );
