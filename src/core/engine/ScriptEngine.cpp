#include "ScriptEngine.h"
#include "js/jsapi.h"
#include "mongo/util/assert_util.h"
#include "mongo/scripting/engine.h"
#include "mongo/scripting/engine_spidermonkey.h"
#include "mongo/shell/shell_utils.h"
#include <QStringList>

#include "js/jsapi.h"
#include "js/jsparse.h"
#include "js/jsscan.h"
#include "js/jsstr.h"
#include "mongo/bson/stringdata.h"

using namespace Robomongo;
using namespace std;

namespace mongo {
    extern bool isShell;
}

ScriptEngine::ScriptEngine(const QString &host, int port, const QString &username, const QString &password, const QString &database) :
    QObject(),
    _host(host),
    _port(port),
    _username(username),
    _password(password),
    _database(database)
{

}

void ScriptEngine::init()
{
    QString url;

    if (_database.isEmpty())
        url = QString("%1:%2").arg(_host).arg(_port);
    else
        url = QString("%1:%2/%3").arg(_host).arg(_port).arg(_database);

    stringstream ss;

    if (_username.isEmpty())
        ss << "db = connect('" << url.toStdString() << "')";
    else
        ss << "db = connect('" << url.toStdString() << "', '"
           << _username.toStdString() << "', '"
           << _password.toStdString() << "')";

    mongo::shell_utils::_dbConnect = ss.str();
    mongo::isShell = true;

    mongo::ScriptEngine::setConnectCallback( mongo::shell_utils::onConnect );
    mongo::ScriptEngine::setup();
    mongo::globalScriptEngine->setScopeInitCallback( mongo::shell_utils::initScope );

    _scope.reset(mongo::globalScriptEngine->newScope());
}

void ScriptEngine::exec(const QString &script)
{
    statementize(script);
    QByteArray array = script.toUtf8();

    if (true /* ! wascmd */) {
        try {
            if ( _scope->exec( array.data() , "(shell)" , false , true , false ) )
                _scope->exec( "shellPrintHelper( __lastres__ );" , "(shell2)" , true , true , false );
        }
        catch ( std::exception& e ) {
            std::cout << "error:" << e.what() << endl;
        }
    }
}

void errorReporter( JSContext *cx, const char *message, JSErrorReport *report ) {
    const char *msg = message;
}

QStringList ScriptEngine::statementize(const QString &script)
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

    JSTokenStream * token_stream;
    JSParseNode * node;

    QByteArray str = script.toUtf8();
//    mongo::StringData str(script.toUtf8());

    size_t length = str.size();
    jschar *chars = js_InflateString(context, str.data(), &length);

    token_stream = js_NewTokenStream(context, chars, length, NULL, 0, NULL);
    if (token_stream == NULL) {
      fprintf(stderr, "cannot create token stream from file\n");
      //exit(EXIT_FAILURE);
    }

    node = js_ParseTokenStream(context, global, token_stream);
    if (node == NULL) {
      fprintf(stderr, "parse error in file\n");
      //exit(EXIT_FAILURE);
    }

    QStringList list;
    parseTree(node, 0, script, list);

    JS_DestroyContext(context);
    JS_DestroyRuntime(runtime);
    return QStringList();
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

void ScriptEngine::parseTree(JSParseNode *root, int indent, const QString &script, QStringList &list)
{
    if (root == NULL) {
      return;
    }
    printf("%*s", indent, "");
    if (root->pn_type >= NUM_TOKENS) {
      printf("UNKNOWN");
    }
    else {

        QStringList lines = script.split(QRegExp("[\r\n]"));

        QString s = subb(lines, root->pn_pos.begin.lineno, root->pn_pos.begin.index, root->pn_pos.end.lineno, root->pn_pos.end.index);
        list.append(s);

      printf("%s: starts at line %d, column %d, ends at line %d, column %d",
             TOKENS[root->pn_type],
             root->pn_pos.begin.lineno, root->pn_pos.begin.index,
             root->pn_pos.end.lineno, root->pn_pos.end.index);
    }
    printf("\n");
    switch (root->pn_arity) {
    case PN_UNARY:
      //parseTree(root->pn_kid, indent + 2, script, list);
      break;
    case PN_BINARY:
      //parseTree(root->pn_left, indent + 2, script, list);
     // parseTree(root->pn_right, indent + 2, script, list);
      break;
    case PN_TERNARY:
   //   parseTree(root->pn_kid1, indent + 2, script, list);
     // parseTree(root->pn_kid2, indent + 2, script, list);
     // parseTree(root->pn_kid3, indent + 2, script, list);
      break;
    case PN_LIST:
      {
        JSParseNode * p;
        for (p = root->pn_head; p != NULL; p = p->pn_next) {
          parseTree(p, indent + 2, script, list);
        }
      }
      break;
    case PN_FUNC:
    case PN_NAME:
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
        if (fline == i)
            buf.append(list.at(i).mid(fpos));
        else
        if (tline == i)
            buf.append(list.at(i).mid(0, tpos));
        else
            buf.append(list.at(i));
    }

    return buf;
}


