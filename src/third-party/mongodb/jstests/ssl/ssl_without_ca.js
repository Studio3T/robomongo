// Must turn these off so we don't have CA file supplied automatically.
TestData.usex509 = false;
TestData.useSSL = false;

var SERVER_CERT = "jstests/libs/server.pem";
var CLIENT_CERT = "jstests/libs/client.pem";
var CLIENT_USER = "C=US,ST=New York,L=New York City,O=MongoDB,OU=KernelUser,CN=client";

jsTest.log("Assert x509 auth is not allowed when a standalone mongod is run without a CA file.");

// allowSSL instead of requireSSL so that the non-SSL connection succeeds.
var conn = MongoRunner.runMongod({sslMode: 'allowSSL',
                                  sslPEMKeyFile: SERVER_CERT,
                                  auth: ''});

var external = conn.getDB('$external');
external.createUser({
    user: CLIENT_USER,
    roles: [
        {'role':'userAdminAnyDatabase', 'db':'admin'},
        {'role':'readWriteAnyDatabase', 'db':'admin'}
    ]});

// Should not be able to authenticate with x509.
// Authenticate call will return 1 on success, 0 on error.
var exitStatus = runMongoProgram('mongo', '--ssl', '--sslAllowInvalidCertificates',
                                 '--sslPEMKeyFile', CLIENT_CERT,
                                 '--port', conn.port,
                                 '--eval', ('quit(db.getSisterDB("$external").auth({' +
                                            'user: "' + CLIENT_USER + '" ,' +
                                            'mechanism: "MONGODB-X509"}));'
                                           ));

assert.eq(exitStatus, 0, "authentication via MONGODB-X509 without CA succeeded");

MongoRunner.stopMongod(conn.port);

jsTest.log("Assert mongod doesn\'t start with CA file missing and clusterAuthMode=x509.");

var sslParams = {clusterAuthMode: 'x509', sslMode: 'requireSSL', sslPEMKeyFile: SERVER_CERT};
var conn = MongoRunner.runMongod(sslParams);
assert.isnull(conn, "server started with x509 clusterAuthMode but no CA file");

jsTest.log("Assert mongos doesn\'t start with CA file missing and clusterAuthMode=x509.");

assert.throws(function() {
                  new ShardingTest({shards: 1, mongos: 1, verbose: 2,
                                    other: {configOptions: sslParams,
                                            mongosOptions: sslParams,
                                            shardOptions: sslParams}});
              },
              null,
              "mongos started with x509 clusterAuthMode but no CA file");
