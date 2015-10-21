// Test mongod start with FIPS mode enabled
ports = allocatePorts(1);
port1 = ports[0];
var baseName = "jstests_ssl_ssl_fips";


var md = startMongod("--port", port1, "--dbpath",
                     MongoRunner.dataPath + baseName, "--sslMode", "requireSSL",
                     "--sslPEMKeyFile", "jstests/libs/server.pem",
                     "--sslFIPSMode");

var mongo = runMongoProgram("mongo", "--port", port1, "--ssl", "--sslAllowInvalidCertificates",
                            "--sslPEMKeyFile", "jstests/libs/client.pem",
                            "--sslFIPSMode",
                            "--eval", ";");

// if mongo shell didn't start/connect properly
if (mongo != 0) {
    print("mongod failed to start, checking for FIPS support");
    mongoOutput = rawMongoProgramOutput()
    assert(mongoOutput.match(/this version of mongodb was not compiled with FIPS support/) ||
        mongoOutput.match(/FIPS_mode_set:fips mode not supported/))
}
else {
    // verify that auth works, SERVER-18051
    md.getDB("admin").createUser({user: "root", pwd: "root", roles: ["root"]});
    assert(md.getDB("admin").auth("root", "root"), "auth failed");

    // kill mongod
    stopMongod(port1);
}
