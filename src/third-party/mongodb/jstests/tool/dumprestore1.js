// dumprestore1.js

t = new ToolTest( "dumprestore1" );

c = t.startDB( "foo" );
assert.eq( 0 , c.count() , "setup1" );
c.save( { a : 22 } );
assert.eq( 1 , c.count() , "setup2" );

t.runTool( "dump" , "--out" , t.ext );

c.drop();
assert.eq( 0 , c.count() , "after drop" );

t.runTool( "restore" , "--dir" , t.ext );
assert.soon( "c.findOne()" , "no data after sleep" );
assert.eq( 1 , c.count() , "after restore 2" );
assert.eq( 22 , c.findOne().a , "after restore 2" );

// ensure that --collection is used with --db. See SERVER-7721
var ret = t.runTool( "dump" , "--collection" , "col" );
assert.neq( ret, 0, "mongodump should return failure code" );
t.stop();

// Ensure that --db and --collection are provided when filename is "-" (stdin).
ret = t.runTool( "restore" , "--collection" , "coll", "--dir", "-" );
assert.neq( ret, 0, "mongorestore should return failure code" );
t.stop();
ret = t.runTool( "restore" , "--db" , "db", "--dir", "-" );
assert.neq( ret, 0, "mongorestore should return failure code" );
t.stop();
