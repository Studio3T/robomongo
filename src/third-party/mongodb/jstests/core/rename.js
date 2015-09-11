admin = db.getMongo().getDB( "admin" );

a = db.jstests_rename_a;
b = db.jstests_rename_b;
c = db.jstests_rename_c;

a.drop();
b.drop();
c.drop();

a.save( {a: 1} );
a.save( {a: 2} );
a.ensureIndex( {a:1} );
a.ensureIndex( {b:1} );

c.save( {a: 100} );
assert.commandFailed( admin.runCommand( {renameCollection:"test.jstests_rename_a", to:"test.jstests_rename_c"} ) );

assert.commandWorked( admin.runCommand( {renameCollection:"test.jstests_rename_a", to:"test.jstests_rename_b"} ) );
assert.eq( 0, a.find().count() );

assert.eq( 2, b.find().count() );
assert( db.getCollectionNames().indexOf( "jstests_rename_b" ) >= 0 );
assert( db.getCollectionNames().indexOf( "jstests_rename_a" ) < 0 );
assert.eq( 3, db.jstests_rename_b.getIndexes().length );
assert.eq( 0, db.jstests_rename_a.getIndexes().length );

// now try renaming a capped collection

a.drop();
b.drop();
c.drop();

// TODO: too many numbers hard coded here
// this test depends precisely on record size and hence may not be very reliable
// note we use floats to make sure numbers are represented as doubles for both SM and v8, since test relies on record size
db.createCollection( "jstests_rename_a", {capped:true,size:10000} );
for( i = 0.1; i < 10; ++i ) {
    a.save( { i: i } );
}
assert.commandWorked( admin.runCommand( {renameCollection:"test.jstests_rename_a", to:"test.jstests_rename_b"} ) );
assert.eq( 1, b.count( {i:9.1} ) );
printjson( b.stats() );
for( i = 10.1; i < 1000; ++i ) {
    b.save( { i: i } );
}
printjson( b.stats() );
//res = b.find().sort({i:1});
//while (res.hasNext()) printjson(res.next());

assert.eq( 1, b.count( {i:i-1} ) ); // make sure last is there
assert.eq( 0, b.count( {i:9.1} ) ); // make sure early one is gone


assert( db.getCollectionNames().indexOf( "jstests_rename_b"  ) >= 0 );
assert( db.getCollectionNames().indexOf( "jstests_rename_a" ) < 0 );
assert( db.jstests_rename_b.stats().capped );
