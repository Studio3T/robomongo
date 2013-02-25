#include <QVector> // unable to put this include below. doesn't compile on GCC 4.7.2 and Qt 4.8
#include "robomongo/core/engine/ScriptEngine.h"

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

using namespace std;

namespace mongo {
    extern bool isShell;
}

namespace Robomongo
{
    ScriptEngine::ScriptEngine(ConnectionSettings *connection) : QObject(),
        _connection(connection) { }

    ScriptEngine::~ScriptEngine()
    {

    }

    void ScriptEngine::init()
    {
        QString connectDatabase = "test";

        if (_connection->hasEnabledPrimaryCredential())
            connectDatabase = _connection->primaryCredential()->databaseName();

        QString url = QString("%1:%2/%3")
            .arg(_connection->serverHost())
            .arg(_connection->serverPort())
            .arg(connectDatabase);

        stringstream ss;

        if (!_connection->hasEnabledPrimaryCredential())
            ss << "db = connect('" << url.toStdString() << "')";
        else
            ss << "db = connect('" << url.toStdString() << "', '"
               << _connection->primaryCredential()->userName().toStdString() << "', '"
               << _connection->primaryCredential()->userPassword().toStdString() << "')";

        {
            QMutexLocker lock(&_mutex);

            //mongo::shell_utils::_dbConnect = "var z = 56;"; //ss.str();
            mongo::shell_utils::_dbConnect = ss.str();
            mongo::isShell = true;

            mongo::ScriptEngine::setConnectCallback( mongo::shell_utils::onConnect );
            mongo::ScriptEngine::setup();
            mongo::globalScriptEngine->setScopeInitCallback( mongo::shell_utils::initScope );

            mongo::Scope *scope = mongo::globalScriptEngine->newScope();

            _scope.reset(scope);
            _engine.reset(mongo::globalScriptEngine);
        }

        // -- Esprima --

        QFile file(":/robomongo/scripts/esprima.js");
        if(!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Unable to read esprima.js ");

        QTextStream in(&file);
        QString esprima = in.readAll();

        QByteArray esprimaBytes = esprima.toUtf8();

        size_t eslength = esprimaBytes.size();

        bool res = _scope->exec(esprima.toStdString(), "(esprima)", true, true, true);
    }

    MongoShellExecResult ScriptEngine::exec(const QString &originalScript, const QString &dbName)
    {
        /*
         * Replace all commands ('show dbs', 'use db' etc.) with call
         * to shellHelper('show', 'dbs') and so on.
         */
        QByteArray bytes = originalScript.toUtf8();
        std::string stdstr(bytes.constData(), bytes.size());

        pcrecpp::RE re("^(\\w+) (\\w+)$",
            pcrecpp::RE_Options()
               .set_utf8(true)
               .set_caseless(true)
               .set_multiline(true));

        re.GlobalReplace("shellHelper('\\1', '\\2');", &stdstr);
        QString script = QString::fromUtf8(stdstr.c_str());

        /*
         * Statementize (i.e. extract all JavaScript statements from script) and
         * execute each statement one by one
         */
        QStringList statements;
        QString error;
        bool result = statementize(script, statements, error);

        if (!result && statements.count() == 0) {
            statements.append(QString("print(__robomongoResult.error)"));
        }

        QList<MongoShellResult> results;

        use(dbName);

        foreach(QString statement, statements)
        {
            // clear global objects
            __objects.clear();
            __logs.str("");

            QByteArray array = statement.toUtf8();

            if (true /* ! wascmd */) {
                try {
                    QElapsedTimer timer;
                    timer.start();
                    if ( _scope->exec( array.data() , "(shell)" , false , true , false ) )
                        _scope->exec( "__robomongoLastRes = __lastres__; shellPrintHelper( __lastres__ );" , "(shell2)" , true , true , false );

                    qint64 elapsed = timer.elapsed();

                    std::string logs = __logs.str();

                    QString answer = QString::fromUtf8(logs.c_str());
                    QVector<mongo::BSONObj> objs = QVector<mongo::BSONObj>::fromStdVector(__objects);
                    QList<mongo::BSONObj> list = QList<mongo::BSONObj>::fromVector(objs);
                    QList<MongoDocumentPtr> docs = MongoDocument::fromBsonObj(list);

                    if (!answer.isEmpty() || docs.count() > 0)
                        results.append(prepareResult(answer, docs, elapsed));
                }
                catch ( std::exception &e ) {
                    std::cout << "error:" << e.what() << endl;
                }
            }
        }

        return prepareExecResult(results);
    }

    void ScriptEngine::use(const QString &dbName)
    {
        if (!dbName.isEmpty()) {
            // switch to database
            QString useDb = QString("shellHelper.use('%1');").arg(dbName);
            QByteArray useDbArray = useDb.toUtf8();
            _scope->exec(useDbArray.data(), "(usedb)", false, true, false);
        }
    }

    QStringList ScriptEngine::complete(const QString &prefix)
    {
//        if ( prefix.find( '"' ) != string::npos )
//            return;

        try {
            using namespace mongo;
            QStringList results;
            mongo::BSONObj args = BSON( "0" << prefix.toStdString() );

            _scope->invokeSafe( "function callShellAutocomplete(x) {shellAutocomplete(x)}", &args, 0, 1000 );
            mongo::BSONObjBuilder b;
            _scope->append( b , "" , "__autocomplete__" );
            mongo::BSONObj res = b.obj();
            mongo::BSONObj arr = res.firstElement().Obj();

            mongo::BSONObjIterator i( arr );
            while ( i.more() ) {
                mongo::BSONElement e = i.next();
                results.append(QString::fromStdString(e.String()));
            }
            return results;
        }
        catch ( ... ) {
            return QStringList();
        }
        return QStringList();
    }

    MongoShellResult ScriptEngine::prepareResult(const QString &output, const QList<MongoDocumentPtr> objects, qint64 elapsedms)
    {
        char *script =
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
            QString serverAddress = getString("__robomongoServerAddress");
            QString dbName = getString("__robomongoDbName");
            QString collectionName = getString("__robomongoCollectionName");
            mongo::BSONObj query = _scope->getObject("__robomongoQuery");
            mongo::BSONObj fields = _scope->getObject("__robomongoFields");

            int limit = _scope->getNumberInt("__robomongoLimit");
            int skip = _scope->getNumberInt("__robomongoSkip");
            int batchSize = _scope->getNumberInt("__robomongoBatchSize");
            int options = _scope->getNumberInt("__robomongoOptions");

            bool special = _scope->getBoolean("__robomongoSpecial");

            MongoQueryInfo info = MongoQueryInfo(serverAddress, dbName, collectionName,
                                       query, fields, limit, skip, batchSize, options, special);
            return MongoShellResult(output, objects, info, elapsedms);
        }

        return MongoShellResult(output, objects, MongoQueryInfo(), elapsedms);
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

        QString serverName = getString("__robomongoServerAddress");
        bool serverIsValid = _scope->getBoolean("__robomongoServerIsValid");

        QString dbName = getString("__robomongoDbName");
        bool dbIsValid = _scope->getBoolean("__robomongoDbIsValid");

        return MongoShellExecResult(results, serverName, serverIsValid, dbName, dbIsValid);
    }

    QString ScriptEngine::getString(const char *fieldName)
    {
        std::string valueStd = _scope->getString(fieldName);
        QString value = QString::fromUtf8(valueStd.c_str());
        return value;
    }

    bool ScriptEngine::statementize(const QString &script, QStringList &outList, QString &outError)
    {
        using namespace mongo;

        QByteArray array = script.toUtf8();
        _scope->setString("__robomongoEsprima", array);

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
            string error = obj.getField("error").str();
            outError = QString::fromStdString(error);
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

            QString statement = script.mid(from, till - from);
            outList.append(statement);
        }

        //std::string json = result.jsonString();
        return true;
    }

    void errorReporter( JSContext *cx, const char *message, JSErrorReport *report ) {
        const char *msg = message;
    }

    QStringList ScriptEngine::statementize2(const QString &script)
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

        QByteArray str = script.toUtf8();

        size_t length = str.size();
        jschar *chars = js_InflateString(context, str.data(), &length);

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

        QStringList list;
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

    void ScriptEngine::parseTree(JSParseNode *root, int indent, const QString &script, QStringList &list, bool topList)
    {
        QStringList lines = script.split('\n');

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

            QString s = subb(lines, root->pn_pos.begin.lineno, root->pn_pos.begin.index, root->pn_pos.end.lineno, root->pn_pos.end.index);
            list.append(s);

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

                QString s = subb(lines,
                                 root->pn_pos.begin.lineno, root->pn_pos.begin.index,
                                 root->pn_expr->pn_pos.end.lineno, root->pn_expr->pn_pos.end.index);
                list.append(s);
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
                QString s = subb(lines,
                                 root->pn_pos.begin.lineno, root->pn_pos.begin.index,
                                 root->pn_expr->pn_pos.end.lineno, root->pn_expr->pn_pos.end.index);
                list.append(s);
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

    QString ScriptEngine::subb(const QStringList &list, int fline, int fpos, int tline, int tpos)
    {
        QString buf;

        if (fline >= list.length())
            return "";

        if (tline >= list.length())
            return "";

        for (int i = fline; i <= tline; i++) {
            QString line = list.at(i);

            if (fline == i && tline == i)
                buf.append(line.mid(fpos, tpos - fpos));
            else if (fline == i)
                buf.append(line.mid(fpos));
            else if (tline == i)
                buf.append(line.mid(0, tpos));
            else
                buf.append(line);
        }

        return buf;
    }

}



