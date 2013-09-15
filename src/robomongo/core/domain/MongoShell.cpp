#include "robomongo/core/domain/MongoShell.h"

#include "mongo/scripting/engine.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"

namespace Robomongo
{

    MongoShell::MongoShell(MongoServer *server, const ScriptInfo &scriptInfo) :
        QObject(),
        _scriptInfo(scriptInfo),
        _server(server)
    {
    }

    void MongoShell::open(const std::string &script, const std::string &dbName)
    {
        AppRegistry::instance().bus()->publish(new ScriptExecutingEvent(this));
        _scriptInfo.setScript(QtUtils::toQString(script));
        _server->client()->send(new ExecuteScriptRequest(this, query(), dbName));
        LOG_MSG(query());
    }

    std::string MongoShell::query() const 
    {
        return QtUtils::toStdString<std::string>(_scriptInfo.script()); 
    }

    void MongoShell::execute(const std::string &dbName)
    {
        if(_scriptInfo.execute())
        {
            AppRegistry::instance().bus()->publish(new ScriptExecutingEvent(this));
            _server->client()->send(new ExecuteScriptRequest(this, query(), dbName));
        }
        else
        {
            AppRegistry::instance().bus()->publish(new ScriptExecutingEvent(this));
            _scriptInfo.setScript("");
            _server->client()->send(new ExecuteScriptRequest(this,query() , dbName));
        }
        LOG_MSG(query());
    }
    void MongoShell::query(int resultIndex, const MongoQueryInfo &info)
    {
        _server->client()->send(new ExecuteQueryRequest(this, resultIndex, info));
    }

    void MongoShell::autocomplete(const std::string &prefix)
    {
        _server->client()->send(new AutocompleteRequest(this, prefix));
    }

    void MongoShell::stop()
    {
        mongo::Scope::setInterruptFlag(true);
    }

    bool MongoShell::loadFromFile()
    {
        return _scriptInfo.loadFromFile();
    }

    bool MongoShell::saveToFileAs()
    {
        return _scriptInfo.saveToFileAs();
    }

    bool MongoShell::saveToFile()
    {
        return _scriptInfo.saveToFile();
    }

    void MongoShell::handle(ExecuteQueryResponse *event)
    {
        AppRegistry::instance().bus()->publish(new DocumentListLoadedEvent(this, event->resultIndex, event->queryInfo, query(), event->documents));
    }

    void MongoShell::handle(ExecuteScriptResponse *event)
    {
        AppRegistry::instance().bus()->publish(new ScriptExecutedEvent(this, event->result, event->empty));
    }

    void MongoShell::handle(AutocompleteResponse *event)
    {
        AppRegistry::instance().bus()->publish(new AutocompleteResponse(this, event->list, event->prefix));
    }
}
