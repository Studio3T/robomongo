#include "robomongo/core/domain/MongoShell.h"

#include <QApplication>

#include "mongo/scripting/engine.h"

#include "robomongo/core/domain/MongoServer.h"
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
        emit startScriptExecuted();
        _scriptInfo.setScript(QtUtils::toQString(script));

        ExecuteScriptInfo inf(query(), dbName, 0, 0);
        _server->postEventToDataBase(new ExecuteScriptEvent(this,inf));
        LOG_MSG(_scriptInfo.script(), mongo::LL_INFO);
    }

    std::string MongoShell::query() const 
    {
        return QtUtils::toStdString(_scriptInfo.script()); 
    }

    void MongoShell::execute(const std::string &dbName)
    {
        emit startScriptExecuted();
        if (_scriptInfo.execute()) {
            ExecuteScriptInfo inf(query(), dbName, 0, 0);
            _server->postEventToDataBase(new ExecuteScriptEvent(this,inf));
            if(!_scriptInfo.script().isEmpty())
                LOG_MSG(_scriptInfo.script(), mongo::LL_INFO);
        } else {
            _scriptInfo.setScript("");
            ExecuteScriptInfo inf(query(), dbName, 0, 0);
            _server->postEventToDataBase(new ExecuteScriptEvent(this,inf));
        }        
    }

    void MongoShell::autocomplete(const std::string &prefix)
    {
        AutoCompleteInfo inf(prefix);
        _server->postEventToDataBase(new AutoCompleteEvent(this,inf));
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
            emit autoCompleteResponced(QtUtils::toQString(v._prefix), v._list);
        }
        else if(type==static_cast<QEvent::Type>(ExecuteScriptEvent::EventType)){
            ExecuteScriptEvent *ev = static_cast<ExecuteScriptEvent*>(event);
            ExecuteScriptEvent::value_type v = ev->value();
            emit scriptExecuted(v);
        }

        return BaseClass::customEvent(event);
    }
}
