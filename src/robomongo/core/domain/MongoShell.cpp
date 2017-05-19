#include "robomongo/core/domain/MongoShell.h"

#include "mongo/scripting/engine.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/settings/SettingsManager.h"

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
        AppRegistry::instance().bus()->send(_server->worker(), new ExecuteScriptRequest(this, query(), dbName));
        LOG_MSG(_scriptInfo.script(), mongo::logger::LogSeverity::Info());
    }

    std::string MongoShell::query() const 
    {
        return QtUtils::toStdString(_scriptInfo.script()); 
    }

    void MongoShell::execute(const std::string &dbName)
    {
        if (_scriptInfo.execute()) {
            AppRegistry::instance().bus()->publish(new ScriptExecutingEvent(this));
            AppRegistry::instance().bus()->send(_server->worker(), new ExecuteScriptRequest(this, query(), dbName));
            if (!_scriptInfo.script().isEmpty())
                LOG_MSG(_scriptInfo.script(), mongo::logger::LogSeverity::Info());
        } else {
            AppRegistry::instance().bus()->publish(new ScriptExecutingEvent(this));
            _scriptInfo.setScript("");
            AppRegistry::instance().bus()->send(_server->worker(), new ExecuteScriptRequest(this, query() , dbName));
        }
    }

    void MongoShell::query(int resultIndex, const MongoQueryInfo &info)
    {
        AppRegistry::instance().bus()->send(_server->worker(), new ExecuteQueryRequest(this, resultIndex, info));
    }

    void MongoShell::autocomplete(const std::string &prefix)
    {
        AutocompletionMode autocompletionMode = AppRegistry::instance().settingsManager()->autocompletionMode();
        if (autocompletionMode == AutocompleteNone)
            return;
        AppRegistry::instance().bus()->send(_server->worker(), new AutocompleteRequest(this, prefix, autocompletionMode));
    }

    void MongoShell::stop()
    {
        // _server->worker()->interrupt();
        // mongo::Scope::setInterruptFlag(true);
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
        if (event->isError()) {
            AppRegistry::instance().bus()->publish(new DocumentListLoadedEvent(this, event->error()));
            return;
        }

        AppRegistry::instance().bus()->publish(new DocumentListLoadedEvent(this, event->resultIndex, event->queryInfo, query(), event->documents));
    }

    void MongoShell::handle(ExecuteScriptResponse *event)
    {
        if (event->isError()) {
            if (_server->connectionRecord()->isReplicaSet()) {
                AppRegistry::instance().bus()->publish(
                    new ReplicaSetRefreshed(this, event->error(), event->error().replicaSetInfo()));

                AppRegistry::instance().bus()->publish(
                    new ScriptExecutedEvent(this, event->error(), event->timeoutReached()));
                return;
            }
            else {  // single server
                AppRegistry::instance().bus()->publish(
                    new ScriptExecutedEvent(this, event->error(), event->timeoutReached()));
                return;
            }
        }

        AppRegistry::instance().bus()->publish(new ScriptExecutedEvent(this, event->result, event->empty,
                                                                       event->timeoutReached()));
    }

    void MongoShell::handle(AutocompleteResponse *event)
    {
        if (event->isError()) {
            AppRegistry::instance().bus()->publish(new AutocompleteResponse(this, event->error()));
            return;
        }

        AppRegistry::instance().bus()->publish(new AutocompleteResponse(this, event->list, event->prefix));
    }
}
