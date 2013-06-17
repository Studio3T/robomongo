// @file restore.cpp

/**
*    Copyright (C) 2008 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pch.h"

#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>
#include <fcntl.h>
#include <fstream>
#include <set>

#include "mongo/base/initializer.h"
#include "mongo/db/namespacestring.h"
#include "mongo/tools/tool.h"
#include "mongo/util/mmap.h"
#include "mongo/util/stringutils.h"
#include "mongo/db/json.h"
#include "mongo/client/dbclientcursor.h"

using namespace mongo;

namespace po = boost::program_options;

namespace {
    const char* OPLOG_SENTINEL = "$oplog";  // compare by ptr not strcmp
}

class Restore : public BSONTool {
public:

    bool _drop;
    bool _keepIndexVersion;
    bool _restoreOptions;
    bool _restoreIndexes;
    int _w;
    string _curns;
    string _curdb;
    string _curcoll;
    set<string> _users; // For restoring users with --drop
    auto_ptr<Matcher> _opmatcher; // For oplog replay
    Restore() : BSONTool( "restore" ) , _drop(false) {
        add_options()
        ("drop" , "drop each collection before import" )
        ("oplogReplay", "replay oplog for point-in-time restore")
        ("oplogLimit", po::value<string>(), "exclude oplog entries newer than provided timestamp (epoch[:ordinal])")
        ("keepIndexVersion" , "don't upgrade indexes to newest version")
        ("noOptionsRestore" , "don't restore collection options")
        ("noIndexRestore" , "don't restore indexes")
        ("w" , po::value<int>()->default_value(1) , "minimum number of replicas per write" )
        ;
        add_hidden_options()
        ("dir", po::value<string>()->default_value("dump"), "directory to restore from")
        ("indexesLast" , "wait to add indexes (now default)") // left in for backwards compatibility
        ;
        addPositionArg("dir", 1);
    }

    virtual void printExtraHelp(ostream& out) {
        out << "Import BSON files into MongoDB.\n" << endl;
        out << "usage: " << _name << " [options] [directory or filename to restore from]" << endl;
    }

    virtual int doRun() {

        // authenticate
        if (hasParam("dbpath")) {
            warning() << "Skipping authentication checks because we are restoring directly to the "
                "file system using --dbpath" << endl;
        } else {
            enum Auth::Level authLevel = Auth::NONE;
            auth("", &authLevel);
            uassert(15935, "user does not have write access", authLevel == Auth::WRITE);
        }

        boost::filesystem::path root = getParam("dir");

        // check if we're actually talking to a machine that can write
        if (!isMaster()) {
            return -1;
        }

        if (isMongos() && _db == "" && exists(root / "config")) {
            log() << "Cannot do a full restore on a sharded system" << endl;
            return -1;
        }

        _drop = hasParam( "drop" );
        _keepIndexVersion = hasParam("keepIndexVersion");
        _restoreOptions = !hasParam("noOptionsRestore");
        _restoreIndexes = !hasParam("noIndexRestore");
        _w = getParam( "w" , 0 );

        bool doOplog = hasParam( "oplogReplay" );

        if (doOplog) {
            // fail early if errors

            if (_db != "") {
                log() << "Can only replay oplog on full restore" << endl;
                return -1;
            }

            if ( ! exists(root / "oplog.bson") ) {
                log() << "No oplog file to replay. Make sure you run mongodump with --oplog." << endl;
                return -1;
            }


            BSONObj out;
            if (! conn().simpleCommand("admin", &out, "buildinfo")) {
                log() << "buildinfo command failed: " << out["errmsg"].String() << endl;
                return -1;
            }

            StringData version = out["version"].valuestr();
            if (versionCmp(version, "1.7.4-pre-") < 0) {
                log() << "Can only replay oplog to server version >= 1.7.4" << endl;
                return -1;
            }

            string oplogLimit = getParam( "oplogLimit", "" );
            string oplogInc = "0";

            if(!oplogLimit.empty()) {
                size_t i = oplogLimit.find_first_of(':');
                if ( i != string::npos ) {
                    if ( i + 1 < oplogLimit.length() ) {
                        oplogInc = oplogLimit.substr(i + 1);
                    }

                    oplogLimit = oplogLimit.substr(0, i);
                }
                
                if ( ! oplogLimit.empty() ) {
                    _opmatcher.reset( new Matcher( fromjson( string("{ \"ts\": { \"$lt\": { \"$timestamp\": { \"t\": ") + oplogLimit + string(", \"i\": ") + oplogInc + string(" } } } }") ) ) );
                }
            }
        }

        /* If _db is not "" then the user specified a db name to restore as.
         *
         * In that case we better be given either a root directory that
         * contains only .bson files or a single .bson file  (a db).
         *
         * In the case where a collection name is specified we better be
         * given either a root directory that contains only a single
         * .bson file, or a single .bson file itself (a collection).
         */
        drillDown(root, _db != "", _coll != "", true);

        // should this happen for oplog replay as well?
        conn().getLastError(_db == "" ? "admin" : _db);

        if (doOplog) {
            log() << "\t Replaying oplog" << endl;
            _curns = OPLOG_SENTINEL;
            processFile( root / "oplog.bson" );
        }

        return EXIT_CLEAN;
    }

    void drillDown( boost::filesystem::path root, bool use_db, bool use_coll, bool top_level=false ) {
        bool json_metadata = false;
        LOG(2) << "drillDown: " << root.string() << endl;

        // skip hidden files and directories
        if (root.leaf().string()[0] == '.' && root.leaf().string() != ".")
            return;

        if ( is_directory( root ) ) {
            boost::filesystem::directory_iterator end;
            boost::filesystem::directory_iterator i(root);
            boost::filesystem::path indexes;
            while ( i != end ) {
                boost::filesystem::path p = *i;
                i++;

                if (use_db) {
                    if (boost::filesystem::is_directory(p)) {
                        error() << "ERROR: root directory must be a dump of a single database" << endl;
                        error() << "       when specifying a db name with --db" << endl;
                        printHelp(cout);
                        return;
                    }
                }

                if (use_coll) {
                    if (boost::filesystem::is_directory(p) || i != end) {
                        error() << "ERROR: root directory must be a dump of a single collection" << endl;
                        error() << "       when specifying a collection name with --collection" << endl;
                        printHelp(cout);
                        return;
                    }
                }

                // Ignore system.indexes.bson if we have *.metadata.json files
                if ( endsWith( p.string().c_str() , ".metadata.json" ) ) {
                    json_metadata = true;
                }

                // don't insert oplog
                if (top_level && !use_db && p.leaf() == "oplog.bson")
                    continue;

                if ( p.leaf() == "system.indexes.bson" ) {
                    indexes = p;
                } else {
                    drillDown(p, use_db, use_coll);
                }
            }

            if (!indexes.empty() && !json_metadata) {
                drillDown(indexes, use_db, use_coll);
            }

            return;
        }

        if ( endsWith( root.string().c_str() , ".metadata.json" ) ) {
            // Metadata files are handled when the corresponding .bson file is handled
            return;
        }

        if ( ! ( endsWith( root.string().c_str() , ".bson" ) ||
                 endsWith( root.string().c_str() , ".bin" ) ) ) {
            error() << "don't know what to do with file [" << root.string() << "]" << endl;
            return;
        }

        log() << root.string() << endl;

        if ( root.leaf() == "system.profile.bson" ) {
            log() << "\t skipping" << endl;
            return;
        }

        string ns;
        if (use_db) {
            ns += _db;
        }
        else {
            ns = root.parent_path().filename().string();
            if (ns.empty())
                ns = "test";
        }

        verify( ns.size() );

        string oldCollName = root.leaf().string(); // Name of the collection that was dumped from
        oldCollName = oldCollName.substr( 0 , oldCollName.find_last_of( "." ) );
        if (use_coll) {
            ns += "." + _coll;
        }
        else {
            ns += "." + oldCollName;
        }

        log() << "\tgoing into namespace [" << ns << "]" << endl;

        if ( _drop ) {
            if (root.leaf() != "system.users.bson" ) {
                log() << "\t dropping" << endl;
                conn().dropCollection( ns );
            } else {
                // Create map of the users currently in the DB
                BSONObj fields = BSON("user" << 1);
                scoped_ptr<DBClientCursor> cursor(conn().query(ns, Query(), 0, 0, &fields));
                while (cursor->more()) {
                    BSONObj user = cursor->next();
                    _users.insert(user["user"].String());
                }
            }
        }

        BSONObj metadataObject;
        if (_restoreOptions || _restoreIndexes) {
            boost::filesystem::path metadataFile = (root.branch_path() / (oldCollName + ".metadata.json"));
            if (!boost::filesystem::exists(metadataFile.string())) {
                // This is fine because dumps from before 2.1 won't have a metadata file, just print a warning.
                // System collections shouldn't have metadata so don't warn if that file is missing.
                if (!startsWith(metadataFile.leaf().string(), "system.")) {
                    log() << metadataFile.string() << " not found. Skipping." << endl;
                }
            } else {
                metadataObject = parseMetadataFile(metadataFile.string());
            }
        }

        _curns = ns.c_str();
        _curdb = NamespaceString(_curns).db;
        _curcoll = NamespaceString(_curns).coll;

        // If drop is not used, warn if the collection exists.
         if (!_drop) {
             scoped_ptr<DBClientCursor> cursor(conn().query(_curdb + ".system.namespaces",
                                                             Query(BSON("name" << ns))));
             if (cursor->more()) {
                 // collection already exists show warning
                 warning() << "Restoring to " << ns << " without dropping. Restored data "
                              "will be inserted without raising errors; check your server log"
                              << endl;
             }
         }

        if (_restoreOptions && metadataObject.hasField("options")) {
            // Try to create collection with given options
            createCollectionWithOptions(metadataObject["options"].Obj());
        }

        processFile( root );
        if (_drop && root.leaf() == "system.users.bson") {
            // Delete any users that used to exist but weren't in the dump file
            for (set<string>::iterator it = _users.begin(); it != _users.end(); ++it) {
                BSONObj userMatch = BSON("user" << *it);
                conn().remove(ns, Query(userMatch));
            }
            _users.clear();
        }

        if (_restoreIndexes && metadataObject.hasField("indexes")) {
            vector<BSONElement> indexes = metadataObject["indexes"].Array();
            for (vector<BSONElement>::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                createIndex((*it).Obj(), false);
            }
        }
    }

    virtual void gotObject( const BSONObj& obj ) {
        if (_curns == OPLOG_SENTINEL) { // intentional ptr compare
            if (obj["op"].valuestr()[0] == 'n') // skip no-ops
                return;
            
            // exclude operations that don't meet (timestamp) criteria
            if ( _opmatcher.get() && ! _opmatcher->matches ( obj ) ) {
                return;
            }

            string db = obj["ns"].valuestr();
            db = db.substr(0, db.find('.'));

            BSONObj cmd = BSON( "applyOps" << BSON_ARRAY( obj ) );
            BSONObj out;
            conn().runCommand(db, cmd, out);

            // wait for ops to propagate to "w" nodes (doesn't warn if w used without replset)
            if ( _w > 0 ) {
                conn().getLastError(db, false, false, _w);
            }
        }
        else if ( endsWith( _curns.c_str() , ".system.indexes" )) {
            createIndex(obj, true);
        }
        else if (_drop && endsWith(_curns.c_str(), ".system.users") && _users.count(obj["user"].String())) {
            // Since system collections can't be dropped, we have to manually
            // replace the contents of the system.users collection
            BSONObj userMatch = BSON("user" << obj["user"].String());
            conn().update(_curns, Query(userMatch), obj);
            _users.erase(obj["user"].String());
        } else {
            conn().insert( _curns , obj );

            // wait for insert to propagate to "w" nodes (doesn't warn if w used without replset)
            if ( _w > 0 ) {
                conn().getLastErrorDetailed(_curdb, false, false, _w);
            }
        }
    }

private:

    BSONObj parseMetadataFile(string filePath) {
        long long fileSize = boost::filesystem::file_size(filePath);
        ifstream file(filePath.c_str(), ios_base::in);

        scoped_ptr<char> buf(new char[fileSize]);
        file.read(buf.get(), fileSize);
        int objSize;
        BSONObj obj;
        obj = fromjson (buf.get(), &objSize);
        uassert(15934, "JSON object size didn't match file size", objSize == fileSize);
        return obj;
    }

    // Compares 2 BSONObj representing collection options. Returns true if the objects
    // represent different options. Ignores the "create" field.
    bool optionsSame(BSONObj obj1, BSONObj obj2) {
        int nfields = 0;
        BSONObjIterator i(obj1);
        while ( i.more() ) {
            BSONElement e = i.next();
            if (!obj2.hasField(e.fieldName())) {
                if (strcmp(e.fieldName(), "create") == 0) {
                    continue;
                } else {
                    return false;
                }
            }
            nfields++;
            if (e != obj2[e.fieldName()]) {
                return false;
            }
        }
        return nfields == obj2.nFields();
    }

    void createCollectionWithOptions(BSONObj cmdObj) {
        if (!cmdObj.hasField("create") || cmdObj["create"].String() != _curcoll) {
            BSONObjBuilder bo;
            if (!cmdObj.hasField("create")) {
                bo.append("create", _curcoll);
            }

            BSONObjIterator i(cmdObj);
            while ( i.more() ) {
                BSONElement e = i.next();
                if (strcmp(e.fieldName(), "create") == 0) {
                    bo.append("create", _curcoll);
                }
                else {
                    bo.append(e);
                }
            }
            cmdObj = bo.obj();
        }

        BSONObj fields = BSON("options" << 1);
        scoped_ptr<DBClientCursor> cursor(conn().query(_curdb + ".system.namespaces", Query(BSON("name" << _curns)), 0, 0, &fields));

        bool createColl = true;
        if (cursor->more()) {
            createColl = false;
            BSONObj obj = cursor->next();
            if (!obj.hasField("options") || !optionsSame(cmdObj, obj["options"].Obj())) {
                    log() << "WARNING: collection " << _curns << " exists with different options than are in the metadata.json file and not using --drop. Options in the metadata file will be ignored." << endl;
            }
        }

        if (!createColl) {
            return;
        }

        BSONObj info;
        if (!conn().runCommand(_curdb, cmdObj, info)) {
            uasserted(15936, "Creating collection " + _curns + " failed. Errmsg: " + info["errmsg"].String());
        } else {
            log() << "\tCreated collection " << _curns << " with options: " << cmdObj.jsonString() << endl;
        }
    }

    /* We must handle if the dbname or collection name is different at restore time than what was dumped.
       If keepCollName is true, however, we keep the same collection name that's in the index object.
     */
    void createIndex(BSONObj indexObj, bool keepCollName) {
        BSONObjBuilder bo;
        BSONObjIterator i(indexObj);
        while ( i.more() ) {
            BSONElement e = i.next();
            if (strcmp(e.fieldName(), "ns") == 0) {
                NamespaceString n(e.String());
                string s = _curdb + "." + (keepCollName ? n.coll : _curcoll);
                bo.append("ns", s);
            }
            else if (strcmp(e.fieldName(), "v") != 0 || _keepIndexVersion) { // Remove index version number
                bo.append(e);
            }
        }
        BSONObj o = bo.obj();
        LOG(0) << "\tCreating index: " << o << endl;
        conn().insert( _curdb + ".system.indexes" ,  o );

        // We're stricter about errors for indexes than for regular data
        BSONObj err = conn().getLastErrorDetailed(_curdb, false, false, _w);

        if (err.hasField("err") && !err["err"].isNull()) {
            if (err["err"].str() == "norepl" && _w > 1) {
                error() << "Cannot specify write concern for non-replicas" << endl;
            }
            else {
                string errCode;

                if (err.hasField("code")) {
                    errCode = str::stream() << err["code"].numberInt();
                }

                error() << "Error creating index " << o["ns"].String() << ": "
                        << errCode << " " << err["err"] << endl;
            }

            ::abort();
        }

        massert(16441, str::stream() << "Error calling getLastError: " << err["errmsg"],
                err["ok"].trueValue());
    }
};

int main( int argc , char ** argv, char ** envp ) {
    mongo::runGlobalInitializersOrDie(argc, argv, envp);
    Restore restore;
    return restore.main( argc , argv );
}
