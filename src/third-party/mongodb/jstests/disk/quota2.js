// Test for quotaFiles off by one file limit issue - SERVER-3420.

if ( 0 ) { // SERVER-3420

port = allocatePorts( 1 )[ 0 ];

baseName = "jstests_disk_quota2";
dbpath = MongoRunner.dataPath + baseName;

m = startMongod( "--port", port, "--dbpath", MongoRunner.dataPath + baseName, "--quotaFiles", "2", "--smallfiles" );
db = m.getDB( baseName );

big = new Array( 10000 ).toString();

// Insert documents until quota is exhausted.
var coll = db[ baseName ];
var res = coll.insert({ b: big });
while( !res.hasWriteError() ) {
    res = coll.insert({ b: big });
}

// Trigger allocation of an additional file for a 'special' namespace.
for( n = 0; !db.getLastError(); ++n ) {
	db.createCollection( '' + n );
}

// Check that new docs are saved in the .0 file.
for( i = 0; i < n; ++i ) {
    c = db[ ''+i ];
    res = c.insert({ b: big });
    if( !res.hasWriteError() ) {
        assert.eq( 0, c.find()._addSpecial( "$showDiskLoc", true )[ 0 ].$diskLoc.file );
    }
}

}
