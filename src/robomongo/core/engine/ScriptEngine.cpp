#include "robomongo/core/engine/ScriptEngine.h"

#include <QVector> // unable to put this include below. doesn't compile on GCC 4.7.2 and Qt 4.8
#include <QDir>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>
#include <QFile>
#include <QElapsedTimer>
#include <js/jsapi.h>
#include <js/jsparse.h>
#include <js/jsscan.h>
#include <js/jsstr.h>
#include <mongo/util/assert_util.h>
#include <mongo/scripting/engine.h>
#include <mongo/scripting/engine_spidermonkey.h>
#include <mongo/shell/shell_utils.h>
#include <mongo/base/string_data.h>
#include <mongo/client/dbclient.h>
#include <pcre/pcrecpp.h>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/CredentialSettings.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    std::vector<std::string> split(const std::string& s, char seperator)
    {
        std::vector<std::string> output;
        std::string::size_type prev_pos = 0, pos = 0;
        while((pos = s.find(seperator, pos)) != std::string::npos)
        {
            std::string substring( s.substr(prev_pos, pos-prev_pos) );
            output.push_back(substring);
            prev_pos = ++pos;
        }
        output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word
        return output;
    }
}

namespace mongo {
    extern bool isShell;
}

namespace Robomongo
{
    ScriptEngine::ScriptEngine(ConnectionSettings *connection) 
        :_connection(connection),
        _scope(NULL),
        _engine(NULL),
        _mutex(QMutex::Recursive)
    { }

    ScriptEngine::~ScriptEngine()
    {
        QMutexLocker lock(&_mutex);

        if (_scope) {
            delete _scope;
            _scope = NULL;
        }

        if (_engine) {
            delete _engine;
            _engine = NULL;
        }
    }

    void ScriptEngine::init()
    {
        QMutexLocker lock(&_mutex);

        std::string connectDatabase = "test";

        if (_connection->hasEnabledPrimaryCredential())
            connectDatabase = _connection->primaryCredential()->databaseName();

        char url[512] = {0};
        sprintf(url,"%s:%d/%s",_connection->serverHost().c_str(),_connection->serverPort(),connectDatabase.c_str());

        std::stringstream ss;

        if (!_connection->hasEnabledPrimaryCredential())
            ss << "db = connect('" << url << "')";
        else
            ss << "db = connect('" << url << "', '"
               << _connection->primaryCredential()->userName() << "', '"
               << _connection->primaryCredential()->userPassword() << "')";

        {
            //mongo::shell_utils::_dbConnect = "var z = 56;"; //ss.str();
            mongo::shell_utils::_dbConnect = ss.str();
            mongo::isShell = true;

            mongo::ScriptEngine::setConnectCallback( mongo::shell_utils::onConnect );
            mongo::ScriptEngine::setup();
            mongo::globalScriptEngine->setScopeInitCallback( mongo::shell_utils::initScope );

            mongo::Scope *scope = mongo::globalScriptEngine->newScope();
            _scope = scope;
            _engine = mongo::globalScriptEngine;

            //protected settings
            _scope->exec("DBQuery.shellBatchSize = 1000", "(esprima)", true, true, true);

            // Load '.mongorc.js' from user's home directory
            // We are not checking whether file exists, because it will be
            // checked by 'Scope::execFile'.
            std::string mongorcPath = QString("%1/."PROJECT_NAME_LOWERCASE".js").arg(QDir::homePath()).toStdString();
            scope->execFile(mongorcPath, false, false);
        }

        // -- Esprima --
        QFile file(":/robomongo/scripts/esprima.js");
        if(!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Unable to read esprima.js ");

        QTextStream in(&file);
        QString esprima = in.readAll();
        _scope->exec(esprima.toStdString(), "(esprima)", true, true, true);
    }

    MongoShellExecResult ScriptEngine::exec(const std::string &originalScript, const std::string &dbName)
    {
        QMutexLocker lock(&_mutex);

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

        QList<MongoShellResult> results;

        use(dbName);

        foreach(std::string statement, statements)
        {
            // clear global objects
            __objects.clear();
            __type = "";
            __finished = false;
            __logs.str("");

            if (true /* ! wascmd */) {
                try {
                    QElapsedTimer timer;
                    timer.start();
                    if ( _scope->exec( statement , "(shell)" , false , true , false, 3600000 ) )
                        _scope->exec( "__robomongoLastRes = __lastres__; shellPrintHelper( __lastres__ );" , "(shell2)" , true , true , false, 3600000);

                    qint64 elapsed = timer.elapsed();

                    std::string logs = __logs.str();
                    std::string answer = logs.c_str();
                    std::string type = __type.c_str();
                    std::vector<MongoDocumentPtr> docs = MongoDocument::fromBsonObj(__objects);

                    if (!answer.empty() || docs.size() > 0)
                        results.append(prepareResult(type, answer, docs, elapsed));
                }
                catch ( const std::exception &e ) {
                    std::cout << "error:" << e.what() << endl;
                }
            }
        }

        return prepareExecResult(results);
    }

    void ScriptEngine::interrupt()
    {
        mongo::Scope::_interruptFlag = true;
    }

    void ScriptEngine::use(const std::string &dbName)
    {
        QMutexLocker lock(&_mutex);

        if (!dbName.empty()) {
            // switch to database
            char useDb[512]={0};
            sprintf(useDb,"shellHelper.use('%s');",dbName.c_str());
            _scope->exec(useDb, "(usedb)", false, true, false);
        }
    }

    void ScriptEngine::ping()
    {
        QMutexLocker lock(&_mutex);

        QString pingStatement = QString("if (db) { db.runCommand({ping:1}); }");
        QByteArray pingArray = pingStatement.toUtf8();
        _scope->exec(pingArray.data(), "(ping)", false, false, false);
    }

    QStringList ScriptEngine::complete(const std::string &prefix)
    {
//        if ( prefix.find( '"' ) != string::npos )
//            return;

        try {
            using namespace mongo;
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

    MongoShellResult ScriptEngine::prepareResult(const std::string &type, const std::string &output, const std::vector<MongoDocumentPtr> &objects, qint64 elapsedms)
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

            MongoQueryInfo info = MongoQueryInfo(serverAddress, dbName, collectionName,
                                       query, fields, limit, skip, batchSize, options, special);
            return MongoShellResult(type, output, objects, info, elapsedms);
        }

        return MongoShellResult(type, output, objects, MongoQueryInfo(), elapsedms);
    }

    MongoShellExecResult ScriptEngine::prepareExecResult(const QList<MongoShellResult> &results)
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

        return MongoShellExecResult(results, serverName, serverIsValid, dbName, dbIsValid);
    }

    std::string ScriptEngine::getString(const char *fieldName)
    {
        return _scope->getString(fieldName);
    }

    bool ScriptEngine::statementize(const std::string &script, std::vector<std::string> &outList, std::string &outError)
    {
        using namespace mongo;

        _scope->setString("__robomongoEsprima", script.c_str());

        StringData data(
            "var __robomongoResult = {};"
            "try {"
                "__robomongoResult.result = esprima.parse(__robomongoEsprima, { range: true, loc : true });"
            "} catch(e) {"
                "__robomongoResult.error = e.name + ': ' + e.message;"
            "}"
            "__robomongoResult;"
        );

        bool res2 = _scope->exec(data, "(esprima2)", false, true, false);
        BSONObj obj = _scope->getObject("__lastres__");

        if (obj.hasField("error")) {
            outError = obj.getField("error");
            return false;
        }

        BSONObj result = obj.getField("result").Obj();
        vector<BSONElement> v = result.getField("body").Array();

        for(vector<BSONElement>::iterator it = v.begin(); it != v.end(); ++it)
        {
            BSONObj item = (*it).Obj();
            BSONObj loc = item.getField("loc").Obj();
            BSONObj start = loc.getField("start").Obj();
            BSONObj end = loc.getField("end").Obj();

            int startLine = start.getIntField("line");
            int startColumn = start.getIntField("column");
            int endLine = end.getIntField("line");
            int endColumn = end.getIntField("column");

            vector<BSONElement> range = item.getField("range").Array();
            int from = (int) range.at(0).number();
            int till = (int) range.at(1).number();

            std::string statement = script.substr(from, till - from);
            outList.push_back(statement);
        }

        //std::string json = result.jsonString();
        return true;
    }

    void errorReporter( JSContext *cx, const char *message, JSErrorReport *report ) {
        const char *msg = message;
    }

    std::vector<std::string> ScriptEngine::statementize2(const std::string &script)
    {
        JSRuntime * runtime;
        JSContext * context;
        JSObject * global;

        runtime = JS_NewRuntime(8L * 1024L * 1024L);
        if (runtime == NULL) {
          fprintf(stderr, "cannot create runtime");
          //exit(EXIT_FAILURE);
        }

        context = JS_NewContext(runtime, 8192);
        if (context == NULL) {
          fprintf(stderr, "cannot create context");
          //exit(EXIT_FAILURE);
        }

        JS_SetOptions( context , JSOPTION_VAROBJFIX);
        JS_SetErrorReporter( context , errorReporter );

        global = JS_NewObject(context, NULL, NULL, NULL);
        if (global == NULL) {
          fprintf(stderr, "cannot create global object");
          //exit(EXIT_FAILURE);
        }

        if (! JS_InitStandardClasses(context, global)) {
          fprintf(stderr, "cannot initialize standard classes");
          //exit(EXIT_FAILURE);
        }

        //-- Esprima --

        QFile file(":/robomongo/scripts/esprima.js");
        if(!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Unable to read esprima.js ");

        QTextStream in(&file);
        QString esprima = in.readAll();

        QByteArray esprimaBytes = esprima.toUtf8();

        size_t eslength = esprimaBytes.size();
    //    jschar *eschars = js_InflateString(context, esprimaBytes.data(), &eslength);

        jsval ret = JSVAL_VOID;
        JSBool worked = JS_EvaluateScript(context, global, esprimaBytes, eslength, NULL, 0, &ret);

        jsval ret2 = JSVAL_VOID;
        mongo::StringData data("esprima.parse('var answer = 42')");
        //mongo::StringData data("connect()");
        JSBool worked2 = JS_EvaluateScript(context, global, data.rawData(), data.size(), NULL, 0, &ret2);


        // end

        JSTokenStream * ts;
        JSParseNode * node;

        size_t length = script.size();
        jschar *chars = js_InflateString(context, script.c_str(), &length);

        ts = js_NewTokenStream(context, chars, length, NULL, 0, NULL);
        if (ts == NULL) {
          fprintf(stderr, "cannot create token stream from file\n");
          //exit(EXIT_FAILURE);
        }

        bool more_tokens = true;
        std::string sanitized_chars;
        jschar *userbuf_pos = ts->userbuf.ptr; // last copied position

    //    while (more_tokens) {
    //        switch (js_GetToken(context, ts)) {
    //        case TOK_NAME:
    //          {
    //            size_t len = ts->userbuf.ptr - userbuf_pos -
    //              (ts->linebuf.limit - ts->linebuf.ptr + 1);
    //            size_t token_len = ts->tokenbuf.ptr - ts->tokenbuf.base;
    //            len -= token_len;
    //            if (len) {
    //              sanitized_chars.append((char*)userbuf_pos, len * sizeof(jschar));
    //              userbuf_pos += len;
    //              if (*(userbuf_pos-1) == '.') break; // properties or methods
    //            }
    ////            sanitized_chars.append((char *)prefix, prefix_len);
    ////            sanitized_chars.append((char*)userbuf_pos, token_len * sizeof(jschar));
    //            userbuf_pos += token_len;
    //          }
    //          break;
    //        }
    //    }


        node = js_ParseTokenStream(context, global, ts);
        if (node == NULL) {
          fprintf(stderr, "parse error in file\n");
          //exit(EXIT_FAILURE);
        }

        std::vector<std::string> list;
        parseTree(node, 0, script, list, true);

        JS_DestroyContext(context);
        JS_DestroyRuntime(runtime);
        return list;
    }



    const char * TOKENS[81] = {
      "EOF", "EOL", "SEMI", "COMMA", "ASSIGN", "HOOK", "COLON", "OR", "AND",
      "BITOR", "BITXOR", "BITAND", "EQOP", "RELOP", "SHOP", "PLUS", "MINUS", "STAR",
      "DIVOP", "UNARYOP", "INC", "DEC", "DOT", "LB", "RB", "LC", "RC", "LP", "RP",
      "NAME", "NUMBER", "STRING", "OBJECT", "PRIMARY", "FUNCTION", "EXPORT",
      "IMPORT", "IF", "ELSE", "SWITCH", "CASE", "DEFAULT", "WHILE", "DO", "FOR",
      "BREAK", "CONTINUE", "IN", "VAR", "WITH", "RETURN", "NEW", "DELETE",
      "DEFSHARP", "USESHARP", "TRY", "CATCH", "FINALLY", "THROW", "INSTANCEOF",
      "DEBUGGER", "XMLSTAGO", "XMLETAGO", "XMLPTAGC", "XMLTAGC", "XMLNAME",
      "XMLATTR", "XMLSPACE", "XMLTEXT", "XMLCOMMENT", "XMLCDATA", "XMLPI", "AT",
      "DBLCOLON", "ANYNAME", "DBLDOT", "FILTER", "XMLELEM", "XMLLIST", "RESERVED",
      "LIMIT",
    };

    const int NUM_TOKENS = sizeof(TOKENS) / sizeof(TOKENS[0]);

    void ScriptEngine::parseTree(JSParseNode *root, int indent, const std::string &script, std::vector<std::string> &list, bool topList)
    {
        std::vector<std::string> lines = split(script,'\n');

        if (root == NULL) {
          return;
        }
        printf("%*s", indent, "");
        if (root->pn_type >= NUM_TOKENS) {
          printf("UNKNOWN");
        }
        else {

    //        if (root->pn_arity == PN_NAME)
    //            return;

            std::string s = subb(lines, root->pn_pos.begin.lineno, root->pn_pos.begin.index, root->pn_pos.end.lineno, root->pn_pos.end.index);
            list.push_back(s);

          printf("%s: starts at line %d, column %d, ends at line %d, column %d",
                 TOKENS[root->pn_type],
                 root->pn_pos.begin.lineno, root->pn_pos.begin.index,
                 root->pn_pos.end.lineno, root->pn_pos.end.index);

    //      if (root->pn_arity == PN_NAME)
    //          return;
        }
        printf("\n");
        switch (root->pn_arity) {
        case PN_UNARY:
        {
            JSToken token = root->pn_ts->tokens[root->pn_ts->cursor];

        }
    //      parseTree(root->pn_kid, indent + 2, script, list);
          break;
        case PN_BINARY:
    //      parseTree(root->pn_left, indent + 2, script, list);
    //      parseTree(root->pn_right, indent + 2, script, list);
          break;
        case PN_TERNARY:
    //      parseTree(root->pn_kid1, indent + 2, script, list);
    //      parseTree(root->pn_kid2, indent + 2, script, list);
    //      parseTree(root->pn_kid3, indent + 2, script, list);
          break;
        case PN_LIST:
          {
            if (topList)
            {
                JSParseNode * tail = *root->pn_tail;

                JSParseNode * p;
                for (p = root->pn_head; p != NULL; p = p->pn_next) {
                  parseTree(p, indent + 2, script, list, false);
                }
            }
            else
            {
                JSParseNode * tail = *root->pn_tail;

                std::string s = subb(lines,
                                 root->pn_pos.begin.lineno, root->pn_pos.begin.index,
                                 root->pn_expr->pn_pos.end.lineno, root->pn_expr->pn_pos.end.index);
                list.push_back(s);
            }
          }
          break;
        case PN_FUNC:
            {
                JSParseNode * body = root->pn_body;

                //parseTree(root->pn_body, indent + 2, script, list);
            }
            break;
        case PN_NAME:
            {
                std::string s = subb(lines,
                                 root->pn_pos.begin.lineno, root->pn_pos.begin.index,
                                 root->pn_expr->pn_pos.end.lineno, root->pn_expr->pn_pos.end.index);
                list.push_back(s);
            }
            break;
        case PN_NULLARY:
          break;
        default:
          fprintf(stderr, "Unknown node type\n");
          //exit(EXIT_FAILURE);
          break;
        }
    }

    std::string ScriptEngine::subb(const std::vector<std::string> &list, int fline, int fpos, int tline, int tpos) const
    {
        std::string result;
        if (fline < list.size() && tline < list.size())
        {
            for (int i = fline; i <= tline; ++i)
            {
                const std::string& line = list[i];
                if (fline == i && tline == i)
                    result.append(line.substr(fpos, tpos - fpos));
                else if (fline == i)
                    result.append(line.substr(fpos));
                else if (tline == i)
                    result.append(line.substr(0, tpos));
                else
                    result.append(line);
            }
        }
        return result;
    }

}