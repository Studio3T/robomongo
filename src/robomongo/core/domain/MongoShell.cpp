#include "robomongo/core/domain/MongoShell.h"

#include "mongo/scripting/engine.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{

    MongoShell::MongoShell(MongoServer *server, const ScriptInfo &scriptInfo) 
        : QObject(),
        _scriptInfo(scriptInfo),
        _server(server),
        _client(server->client()),
        _bus(AppRegistry::instance().bus())
    {

    }

    MongoShell::~MongoShell()
    {
    }

    void MongoShell::open(const std::string &script, const std::string &dbName)
    {
        _bus->publish(new ScriptExecutingEvent(this));
        _scriptInfo.setScript(QtUtils::toQString(script));
        _client->send(new ExecuteScriptRequest(this, query(), dbName));
    }

    std::string MongoShell::query() const { return QtUtils::toStdString<std::string>(_scriptInfo.script()); }

    void MongoShell::execute(const std::string &dbName)
    {
        if(_scriptInfo.execute())
        {
            _bus->publish(new ScriptExecutingEvent(this));
            _client->send(new ExecuteScriptRequest(this, query(), dbName));
        }
        else
        {
            _bus->publish(new ScriptExecutingEvent(this));
            _scriptInfo.setScript("");
            _client->send(new ExecuteScriptRequest(this,query() , dbName));
        }
    }
    void MongoShell::query(int resultIndex, const MongoQueryInfo &info)
    {
        _client->send(new ExecuteQueryRequest(this, resultIndex, info));
    }

    void MongoShell::autocomplete(const std::string &prefix)
    {
        _client->send(new AutocompleteRequest(this, prefix));
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
        _bus->publish(new DocumentListLoadedEvent(this, event->resultIndex, event->queryInfo, query(), event->documents));
    }

    void MongoShell::handle(ExecuteScriptResponse *event)
    {
        _bus->publish(new ScriptExecutedEvent(this, event->result, event->empty));
    }

    void MongoShell::handle(AutocompleteResponse *event)
    {
        _bus->publish(new AutocompleteResponse(this, event->list, event->prefix));
    }
}
