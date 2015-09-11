// This test checks that w:"majority" works correctly on a lone mongod

// set up a mongod and connect a mongo
port = allocatePorts(1)[0];
var baseName = "single_server_majority";
var mongod = startMongod("--port", port, "--dbpath", MongoRunner.dataPath + baseName, "-v");
var mongo = startMongoProgram("mongo", "--port", port);

// get db and collection, then preform a trivial insert 
db = mongo.getDB("test")
col = db.getCollection("single_server_majority");
col.drop();
// see if we can get a majority write on this single server
assert.writeOK(col.save({ a: "test" }, { writeConcern: { w: 'majority' }}));

