#include "ScriptEngine.h"
#include "js/jsapi.h"
#include "mongo/util/assert_util.h"
#include "mongo/scripting/engine.h"
#include "mongo/scripting/engine_spidermonkey.h"
#include "mongo/shell/shell_utils.h"

using namespace Robomongo;
using namespace std;

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

    mongo::ScriptEngine::setConnectCallback( mongo::shell_utils::onConnect );
    mongo::ScriptEngine::setup();
    mongo::globalScriptEngine->setScopeInitCallback( mongo::shell_utils::initScope );

    _scope.reset(mongo::globalScriptEngine->newScope());
}

void ScriptEngine::exec(const QString &script)
{
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
