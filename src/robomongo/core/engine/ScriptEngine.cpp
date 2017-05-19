#include "robomongo/core/engine/ScriptEngine.h"

#include <QVector> // unable to put this include below. doesn't compile on GCC 4.7.2 and Qt 4.8
#include <QDir>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>
#include <QFile>
#include <QElapsedTimer>

// v0.9
//#include <third_party/js-1.7/jsapi.h>
//#include <third_party/js-1.7/jsparse.h>
//#include <third_party/js-1.7/jsscan.h>
//#include <third_party/js-1.7/jsstr.h>

#include <mongo/util/assert_util.h>
#include <mongo/util/exit_code.h>
#include <mongo/scripting/engine.h>

// v0.9
//#include <mongo/scripting/engine_spidermonkey.h>
#include <mongo/scripting/mozjs/engine.h>

#include <mongo/shell/shell_utils.h>
#include <mongo/base/string_data.h>
#include <mongo/client/dbclientinterface.h>
#include <pcrecpp.h>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    std::vector<std::string> split(const std::string &s, char seperator)
    {
        std::vector<std::string> output;
        std::string::size_type prev_pos = 0, pos = 0;
        while ((pos = s.find(seperator, pos)) != std::string::npos) {
            std::string substring(s.substr(prev_pos, pos-prev_pos));
            output.push_back(substring);
            prev_pos = ++pos;
        }
        output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word
        return output;
    }
}

namespace mongo {
    extern bool isShell;
    void logProcessDetailsForLogRotate() {}
}

namespace Robomongo
{
    ScriptEngine::ScriptEngine(ConnectionSettings *connection, int timeoutSec) :
        _connection(connection),
        _scope(nullptr),
        _engine(NULL),
        _timeoutSec(timeoutSec),
        _initialized(false),
        _mutex(QMutex::Recursive) { }

    ScriptEngine::~ScriptEngine()
    {
    }

    void ScriptEngine::init(bool isLoadMongoRcJs, const std::string& serverAddr, const std::string& dbName)
    {
        QMutexLocker lock(&_mutex);

        std::string connectDatabase = dbName.empty() ? "test" : dbName;

        if (_connection->hasEnabledPrimaryCredential())
            connectDatabase = _connection->primaryCredential()->databaseName();

        std::stringstream ss;
        auto hostAndPort = serverAddr.empty() ? _connection->hostAndPort().toString() : serverAddr;
        ss << "db = connect('" << hostAndPort << "/" << connectDatabase;

//        v0.9
//        ss << "db = connect('" << _connection->serverHost() << ":" << _connection->serverPort() << _connection->sslInfo() << _connection->sshInfo() << "/" << connectDatabase;

        if (!_connection->hasEnabledPrimaryCredential())
            ss << "')";
        else
            ss << "', '"
               << _connection->primaryCredential()->userName() << "', '"
               << _connection->primaryCredential()->userPassword() << "')";

        {
            mongo::shell_utils::_dbConnect = ss.str();
            mongo::shell_utils::_dbAuth = "(function() { \nDB.prototype._defaultGssapiServiceName = \"mongodb\";\n}())";

            // v0.9
            // mongo::isShell = true;

            mongo::ScriptEngine::setConnectCallback( mongo::shell_utils::onConnect );
            mongo::ScriptEngine::setup();            
            mongo::getGlobalScriptEngine()->setScopeInitCallback(mongo::shell_utils::initScope);
            mongo::getGlobalScriptEngine()->enableJIT(true);

            _scope.reset(mongo::getGlobalScriptEngine()->newScope());
            _engine = mongo::getGlobalScriptEngine();

            // Load '.mongorc.js' from user's home directory
            if (isLoadMongoRcJs) {
                QString mongorcPath = QString("%1/.mongorc.js").arg(QDir::homePath());
                if (QFile::exists(mongorcPath)) {
                    _scope->execFile(QtUtils::toStdString(mongorcPath), false, false);
                }
            }

            // Load '.robomongorc.js'
            QString robomongorcPath = QString("%1/.robomongorc.js").arg(QDir::homePath());
            if (QFile::exists(robomongorcPath)) {
                _scope->execFile(QtUtils::toStdString(robomongorcPath), false, false);
            }
            _failedScope = false;
        }

        // Esprima ECMAScript parser: http://esprima.org/
        std::string esprima = loadFile(":/robomongo/scripts/esprima.js", true);
        _scope->exec(esprima, "(esprima)", false, true, true);

        // UUID helpers
        std::string uuidhelpers = loadFile(":/robomongo/scripts/uuidhelpers.js", true);
        _scope->exec(uuidhelpers, "(uuidhelpers)", false, true, true);

        // Enable verbose shell reporting
        _scope->exec("_verboseShell = true;", "(verboseShell)", false, false, false);

        // Save original autocomplete function so it can be restored if overwritten by user preference
        _scope->exec("DB.autocompleteOriginal = DB.autocomplete;", "(saveOriginalAutocomplete)", false, false, false);

        // Cache result of original "DB.autocomplete"
        // Cache invalidated by the invalidateDbCollectionsCache() method.
        std::string cacheAutocompletion =
            "__robomongoAutocompletionCache = null;"
            "DB.autocompleteCached = function(obj) { "
            "   if (__robomongoAutocompletionCache == null) {"
            "       __robomongoAutocompletionCache = DB.autocompleteOriginal(obj);"
            "   }"
            "   return __robomongoAutocompletionCache;"
            "}";

        _scope->exec(cacheAutocompletion, "", false, false, false);

        _initialized = true;
    }

    MongoShellExecResult ScriptEngine::exec(const std::string &originalScript, const std::string &dbName)
    {
        QMutexLocker lock(&_mutex);

        if (!_scope) {
            _failedScope = true;
            return MongoShellExecResult(true, "Connection error. Uninitialized mongo scope.");
        }

        // robomongo shell timeout
        bool timeoutReached = false;

        /*
         * Replace all commands ('show dbs', 'use db' etc.) with call
         * to shellHelper('show', 'dbs') and so on.
         */
        std::string stdstr(originalScript);

        pcrecpp::RE re("^(show|use|set) (\\w+)$",
            pcrecpp::RE_Options(PCRE_CASELESS|PCRE_MULTILINE|PCRE_NEWLINE_ANYCRLF));

        re.GlobalReplace("shellHelper('\\1', '\\2');", &stdstr);

        /*
         * Statementize (i.e. extract all JavaScript statements from script) and
         * execute each statement one by one
         */
        std::vector<std::string> statements;
        std::string error;
        bool result = statementize(stdstr, statements, error);

        if (!result && statements.size() == 0) {
            statements.push_back("print(__robomongoResult.error)");
        }

        std::vector<MongoShellResult> results;

        use(dbName);

        for (std::vector<std::string>::const_iterator it = statements.begin(); it != statements.end(); ++it)
        {
            std::string statement = *it;
            // clear global objects
            __objects.clear();
            __type = "";
            __finished = false;
            __logs.str("");

            if (true /* ! wascmd */) {
                try {
                    bool failed = false;
                    QElapsedTimer timer;
                    timer.start();
                    if ( _scope->exec( statement , "(shell)" , false , true , false, _timeoutSec * 1000) ) {
                         _scope->exec( "__robomongoLastRes = __lastres__; shellPrintHelper( __lastres__ );", 
                                      "(shell2)" , true , true , false, _timeoutSec * 1000);
                    }
                    else   // failed to run script 
                        failed = true;                                            

                    qint64 elapsed = timer.elapsed();   // milliseconds 

                    if (elapsed > _timeoutSec * 1000)
                        timeoutReached = true;

                    std::string logs = __logs.str();
                    std::string answer = logs.c_str();
                    std::string type = __type.c_str();

                    if (failed && !timeoutReached)
                        return MongoShellExecResult(true, answer);

                    std::vector<MongoDocumentPtr> docs = MongoDocument::fromBsonObj(__objects);

                    if (!answer.empty() || docs.size() > 0)
                        results.push_back(prepareResult(type, answer, docs, elapsed));
                }
                catch (const std::exception &e) {
                    std::cout << "error:" << e.what() << std::endl;
                }
            }
        }

        return prepareExecResult(results, timeoutReached);
    }

    void ScriptEngine::interrupt()
    {
        // This operation crash Robomongo
        // static_cast<mongo::mozjs::MozJSImplScope*>(_scope)->kill();

        // v0.9
        //mongo::Scope::_interruptFlag = true;
    }

    void ScriptEngine::use(const std::string &dbName)
    {
        QMutexLocker lock(&_mutex);

        if (!dbName.empty()) {
            std::stringstream ss;

            // Switch to database
            ss << "shellHelper.use('" << dbName << "');" << std::endl;

            // Always allow to read from slave
            ss << "rs.slaveOk();" << std::endl;

            _scope->exec(ss.str(), "(usedb)", false, true, false);
        }
    }

    void ScriptEngine::setBatchSize(int batchSize)
    {
        QMutexLocker lock(&_mutex);

        char buff[64] = {0};
        sprintf(buff, "DBQuery.shellBatchSize = %d", batchSize);

        _scope->exec(buff, "(shellBatchSize)", false, true, true);
    }

    void ScriptEngine::ping()
    {
        if (!_scope)
            return;

        QMutexLocker lock(&_mutex);
        _scope->exec("if (db) { db.runCommand({ping:1}); }", "(ping)", false, false, false, 3000);
    }

    QStringList ScriptEngine::complete(const std::string &prefix, const AutocompletionMode mode)
    {
        //if ( prefix.find( '"' ) != string::npos )
        //    return;

        try {
            if (mode == AutocompleteAll)
                _scope->exec("DB.autocomplete = DB.autocompleteCached;", "", false, false, false);
            else if (mode == AutocompleteNoCollectionNames)
                _scope->exec("DB.autocomplete = function(obj){return [];}", "", false, false, false);

            QStringList results;
            mongo::BSONObj args = BSON( "0" << prefix );

            _scope->invokeSafe( "function callShellAutocomplete(x) {shellAutocomplete(x)}", &args, 0, 1000 );
            mongo::BSONObjBuilder b;
            _scope->append( b , "" , "__autocomplete__" );
            mongo::BSONObj res = b.obj();
            mongo::BSONObj arr = res.firstElement().Obj();

            mongo::BSONObjIterator i( arr );
            while ( i.more() ) {
                mongo::BSONElement e = i.next();
                results.append(QtUtils::toQString(e.String()));
            }
            return results;
        }
        catch ( ... ) {
            return QStringList();
        }
        return QStringList();
    }

    MongoShellResult ScriptEngine::prepareResult(const std::string &type, const std::string &output,
                                                 const std::vector<MongoDocumentPtr> &objects, qint64 elapsedms)
    {
        const char *script =
            "__robomongoQuery = false; \n"
            "__robomongoDbName = '[invalid database]'; \n"
            "__robomongoServerAddress = '[invalid connection]'; \n"
            "__robomongoCollectionName = '[invalid collection]'; \n"
            "if (typeof __robomongoLastRes == 'object' && __robomongoLastRes != null && __robomongoLastRes instanceof DBQuery) { \n"
            "    __robomongoQuery = true; \n"
            "    __robomongoDbName = __robomongoLastRes._db.getName();\n "
            "    __robomongoServerAddress = __robomongoLastRes._mongo.host; \n"
            "    __robomongoCollectionName = __robomongoLastRes._collection._shortName; \n"
            "    __robomongoQuery = __robomongoLastRes._query; \n"
            "    __robomongoFields = __robomongoLastRes._fields; \n"
            "    __robomongoLimit = __robomongoLastRes._limit; \n"
            "    __robomongoSkip = __robomongoLastRes._skip; \n"
            "    __robomongoBatchSize = __robomongoLastRes._batchSize; \n"
            "    __robomongoOptions = __robomongoLastRes._options; \n"
            "    __robomongoSpecial = __robomongoLastRes._special; \n"
            "} \n";

        _scope->exec(script, "(getresultinfo)", false, false, false);

        bool isQuery = _scope->getBoolean("__robomongoQuery");

        if (isQuery) {
            std::string serverAddress = getString("__robomongoServerAddress");
            std::string dbName = getString("__robomongoDbName");
            std::string collectionName = getString("__robomongoCollectionName");
            mongo::BSONObj query = _scope->getObject("__robomongoQuery");
            mongo::BSONObj fields = _scope->getObject("__robomongoFields");

            int limit = _scope->getNumberInt("__robomongoLimit");
            int skip = _scope->getNumberInt("__robomongoSkip");
            int batchSize = _scope->getNumberInt("__robomongoBatchSize");
            int options = _scope->getNumberInt("__robomongoOptions");

            bool special = _scope->getBoolean("__robomongoSpecial");

            MongoQueryInfo info = MongoQueryInfo(CollectionInfo(serverAddress, dbName, collectionName),
                                       query, fields, limit, skip, batchSize, options, special);
            return MongoShellResult(type, output, objects, info, elapsedms);
        }

        return MongoShellResult(type, output, objects, MongoQueryInfo(), elapsedms);
    }

    MongoShellExecResult ScriptEngine::prepareExecResult(const std::vector<MongoShellResult> &results, 
                                                         bool timeoutReached /* = false */)
    {
        const char *script =
            "__robomongoServerAddress = '[invalid connection]'; \n"
            "__robomongoServerIsValid = false; \n"
            "__robomongoDbName = '[invalid database]'; \n"
            "__robomongoDbIsValid = false; \n"
            "if (typeof db == 'object' && db != null && db instanceof DB) { \n"
            "    __robomongoServerAddress = db.getMongo().host; \n"
            "    __robomongoServerIsValid = true; \n"
            "    __robomongoDbName = db.getName();\n "
            "    __robomongoDbIsValid = true; \n "
            "} \n";

        _scope->exec(script, "(getdbname)", false, false, false);

        std::string serverName = getString("__robomongoServerAddress");
        bool serverIsValid = _scope->getBoolean("__robomongoServerIsValid");

        std::string dbName = getString("__robomongoDbName");
        bool dbIsValid = _scope->getBoolean("__robomongoDbIsValid");

        return MongoShellExecResult(results, serverName, serverIsValid, dbName, dbIsValid, timeoutReached);
    }

    std::string ScriptEngine::getString(const char *fieldName)
    {
        return _scope->getString(fieldName);
    }

    bool ScriptEngine::statementize(const std::string &script, std::vector<std::string> &outList, std::string &outError)
    {
        QString qScript = QtUtils::toQString(script);
        _scope->setString("__robomongoEsprima", script.c_str());

        mongo::StringData data(
            "var __robomongoResult = {};"
            "try {"
                "__robomongoResult.result = esprima.parse(__robomongoEsprima, { range: true, loc : true });"
            "} catch(e) {"
                "__robomongoResult.error = e.name + ': ' + e.message;"
            "}"
            "__robomongoResult;"
        );

        bool res2 = _scope->exec(data, "(esprima2)", false, true, false);
        mongo::BSONObj obj = _scope->getObject("__lastres__");

        if (obj.hasField("error")) {
            outError = obj.getField("error");
            return false;
        }

        mongo::BSONObj result = obj.getField("result").Obj();
        std::vector<mongo::BSONElement> v = result.getField("body").Array();
        for (std::vector<mongo::BSONElement>::iterator it = v.begin(); it != v.end(); ++it)
        {
            mongo::BSONObj item = (*it).Obj();
            mongo::BSONObj loc = item.getField("loc").Obj();
            mongo::BSONObj start = loc.getField("start").Obj();
            mongo::BSONObj end = loc.getField("end").Obj();

            int startLine = start.getIntField("line");
            int startColumn = start.getIntField("column");
            int endLine = end.getIntField("line");
            int endColumn = end.getIntField("column");

            std::vector<mongo::BSONElement> range = item.getField("range").Array();
            int from = (int) range.at(0).number();
            int till = (int) range.at(1).number();

            std::string statement = QtUtils::toStdString(qScript.mid(from, till - from));
            outList.push_back(statement);
        }

        return true;
    }

    void ScriptEngine::invalidateDbCollectionsCache() {
        if (!_initialized)
            return;

        _scope->exec("__robomongoAutocompletionCache = null;", "", false, false, false);
    }

    std::string ScriptEngine::loadFile(const QString &path, bool throwOnError) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            if (throwOnError)
                throw std::runtime_error("Unable to load file");

            return "";
        }

        QTextStream in(&file);
        QString content = in.readAll();
        return QtUtils::toStdString(content);
    }
}
