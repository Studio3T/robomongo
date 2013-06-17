// SERVER-2111 Check that an in memory db name will block creation of a db with a similar but differently cased name.

a = db.getSisterDB( "dbcase2test_dbnamea" )
b = db.getSisterDB( "dbcase2test_dbnameA" )

a.c.count();
assert.throws( function() { b.c.count() } );

assert.eq( -1, db.getMongo().getDBNames().indexOf( "dbcase2test_dbnameA" ) );
