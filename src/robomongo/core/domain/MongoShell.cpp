#include "robomongo/core/domain/MongoShell.h"

#include <QApplication>

#include "mongo/scripting/engine.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/events/MongoEventsGui.hpp"

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

        ExecuteScriptInfo inf(query(), dbName, 0, 0);
        qApp->postEvent(_server->client(), new ExecuteScriptEvent(this,inf));

        LOG_MSG(_scriptInfo.script(), mongo::LL_INFO);
    }

    std::string MongoShell::query() const 
    {
        return QtUtils::toStdString(_scriptInfo.script()); 
    }

    void MongoShell::execute(const std::string &dbName)
    {
        if (_scriptInfo.execute()) {
            AppRegistry::instance().bus()->publish(new ScriptExecutingEvent(this));
            ExecuteScriptInfo inf(query(), dbName, 0, 0);
            qApp->postEvent(_server->client(), new ExecuteScriptEvent(this,inf));
            if(!_scriptInfo.script().isEmpty())
                LOG_MSG(_scriptInfo.script(), mongo::LL_INFO);
        } else {
            AppRegistry::instance().bus()->publish(new ScriptExecutingEvent(this));
            _scriptInfo.setScript("");
            ExecuteScriptInfo inf(query(), dbName, 0, 0);
            qApp->postEvent(_server->client(), new ExecuteScriptEvent(this,inf));
        }        
    }

    void MongoShell::query(int resultIndex, const MongoQueryInfo &info)
    {
        ExecuteQueryInfo inf(resultIndex,info);
        qApp->postEvent(_server->client(), new ExecuteQueryEvent(this,inf));
    }

    void MongoShell::autocomplete(const std::string &prefix)
    {
        AutoCompleteInfo inf(prefix);
        qApp->postEvent(_server->client(), new AutoCompleteEvent(this,inf));
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

    void MongoShell::customEvent(QEvent *event)
    {
        QEvent::Type type = event->type();
        if(type==static_cast<QEvent::Type>(AutoCompleteEvent::EventType)){
            AutoCompleteEvent *ev = static_cast<AutoCompleteEvent*>(event);
            AutoCompleteEvent::value_type v = ev->value();
            emit autoCompleateResponced(QtUtils::toQString(v._prefix), v._list);
        }
        else if(type==static_cast<QEvent::Type>(ExecuteQueryEvent::EventType)){
            ExecuteQueryEvent *ev = static_cast<ExecuteQueryEvent*>(event);
            ExecuteQueryEvent::value_type v = ev->value();

            AppRegistry::instance().bus()->publish(new DocumentListLoadedEvent(this, v._resultIndex, v._queryInfo, query(), v._documents));
        }
        else if(type==static_cast<QEvent::Type>(ExecuteScriptEvent::EventType)){
            ExecuteScriptEvent *ev = static_cast<ExecuteScriptEvent*>(event);
            ExecuteScriptEvent::value_type v = ev->value();

            AppRegistry::instance().bus()->publish(new ScriptExecutedEvent(this, v._result, v._empty));
        }

        return BaseClass::customEvent(event);
    }
}
