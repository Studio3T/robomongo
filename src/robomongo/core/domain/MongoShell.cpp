#include "robomongo/core/domain/MongoShell.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
namespace Robomongo
{

    MongoShell::MongoShell(MongoServer *server, const ScriptInfo &scriptInfo) :
        QObject(),
        _scriptInfo(scriptInfo),
        _server(server),
        _client(server->client()),
        _bus(AppRegistry::instance().bus())
    {

    }

    MongoShell::~MongoShell()
    {
    }

    void MongoShell::open(const QString &script, const QString &dbName)
    {
        _bus->publish(new ScriptExecutingEvent(this));
		_scriptInfo.setScript(script);
        _client->send(new ExecuteScriptRequest(this, query(), dbName));
    }
    void MongoShell::execute(const QString &dbName)
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

    void MongoShell::autocomplete(const QString &prefix)
    {
        _client->send(new AutocompleteRequest(this, prefix));
    }

    void MongoShell::stop()
    {
        mongo::Scope::setInterruptFlag(true);
    }
	void MongoShell::loadFromFile()
	{
        _scriptInfo.loadFromFile();
	}
	void MongoShell::saveToFileAs()
	{
		_scriptInfo.saveToFileAs();
	}
	void MongoShell::saveToFile()
	{
		_scriptInfo.saveToFile();
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
