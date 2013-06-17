var st = new ShardingTest({ keyFile : 'jstests/libs/key1', shards : 2, chunksize : 1, config : 3, verbose : 2,
                            other : { nopreallocj : 1, verbose : 2, useHostname : true,
                                      configOptions : { verbose : 2 }}});

st.printShardingStatus();

var mongos = st.s;
var adminDB = mongos.getDB('admin');
var db = mongos.getDB('test')


// SERVER-6591: can't add first admin user even when connected to mongos on localhost.
var addUser = function( db, username, password ) {
    var conn = db.getMongo();
    // Get a connection over localhost so that the first user can be added.
    if ( conn.host.indexOf('localhost') != 0 && conn.host.split(',').length > 1 ) {
        print( 'Getting locahost connection instead of ' + conn + ' to add user' );
        var hosts = conn.host.split(',');
        for ( var i = 0; i < hosts.length; i++ ) {
            conn = new Mongo( 'localhost:' + hosts[i].split(':')[1] );
            print( "Adding user on connection: " + conn );
            if ( !conn.getDB('admin').addUser( username, password ) ) {
                return false;
            }
        }
        return true;
    } else {
        return conn.getDB('admin').addUser( username, password );
    }
}


addUser( st._configConnection.getDB('admin'), 'admin', 'password' );

jsTestLog( "Add user was successful" );


// Test for SERVER-6549, make sure that repeatedly logging in always passes.
for ( var i = 0; i < 100; i++ ) {
    adminDB = new Mongo( mongos.host ).getDB('admin');
    assert( adminDB.auth('admin', 'password'), "Auth failed on attempt #: " + i );
}

st.stop();