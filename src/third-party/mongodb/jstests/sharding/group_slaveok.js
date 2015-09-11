// Tests group using slaveOk

var st = new ShardingTest( testName = "groupSlaveOk",
                           numShards = 1,
                           verboseLevel = 0,
                           numMongos = 1,
                           { rs : true, 
                             rs0 : { nodes : 2 }
                           })

var rst = st._rs[0].test

// Insert data into replica set
var conn = new Mongo( st.s.host )
conn.setLogLevel( 3 )

var coll = conn.getCollection( "test.groupSlaveOk" )
coll.drop()

var bulk = coll.initializeUnorderedBulkOp();
for( var i = 0; i < 300; i++ ){
    bulk.insert( { i : i % 10 } );
}
assert.writeOK( bulk.execute() );

st.printShardingStatus()

// Wait for client to update itself and replication to finish
rst.awaitReplication()

var primary = rst.getPrimary()
var sec = rst.getSecondary()

// Data now inserted... stop the master, since only two in set, other will still be secondary
rst.stop(rst.getMaster());
printjson( rst.status() )

// Wait for the mongos to recognize the slave
ReplSetTest.awaitRSClientHosts( conn, sec, { ok : true, secondary : true } )

// Need to check slaveOk=true first, since slaveOk=false will destroy conn in pool when
// master is down
conn.setSlaveOk()

// Should not throw exception, since slaveOk'd
assert.eq( 10, coll.group({ key : { i : true } , 
                            reduce : function( obj, ctx ){ ctx.count += 1 } ,
                            initial : { count : 0 } }).length )

try {
   
    conn.setSlaveOk( false ) 
    var res = coll.group({ key : { i : true } , 
                           reduce : function( obj, ctx ){ ctx.count += 1 } ,
                           initial : { count : 0 } });

    print( "Should not reach here! Group result: " + tojson(res) );
    assert( false );
}
catch( e ){
    print( "Non-slaveOk'd connection failed." + tojson(e) )
}

// Finish
st.stop()
