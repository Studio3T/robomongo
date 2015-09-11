// Test auto reclone after failed initial clone

soonCountAtLeast = function( db, coll, count ) {
    assert.soon( function() { 
//                print( "count: " + s.getDB( db )[ coll ].find().count() );
                return s.getDB( db )[ coll ].find().itcount() >= count;
                } );    
}

doTest = function(signal, extraOpts) {

    rt = new ReplTest( "repl5tests" );
    
    m = rt.start( true );
    
    ma = m.getDB( "a" ).a;
    var bulk = ma.initializeUnorderedBulkOp();
    for( i = 0; i < 10000; ++i )
        bulk.insert({ i: i });
    assert.writeOK(bulk.execute());
    
    s = rt.start(false, extraOpts);
    soonCountAtLeast( "a", "a", 1 );
    rt.stop( false, signal );

    s = rt.start(false, extraOpts, true);
    sleep( 1000 );
    soonCountAtLeast( "a", "a", 10000 );

    rt.stop();
}

doTest( 15 ); // SIGTERM
doTest(9, { journal: null });  // SIGKILL
