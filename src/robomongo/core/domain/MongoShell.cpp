#include "robomongo/core/domain/MongoShell.h"

#include <QApplication>

#include "mongo/scripting/engine.h"

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/events/MongoEvents.hpp"

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
        _scriptInfo.setScript(QtUtils::toQString(script));

        EventsInfo::ExecuteScriptRequestInfo inf(query(), dbName, 0, 0);
        emit startedScriptExecuted(inf);
        _server->postEventToDataBase(new Events::ExecuteScriptRequestEvent(this,inf));
        LOG_MSG(_scriptInfo.script(), mongo::LL_INFO);
    }

    std::string MongoShell::query() const 
    {
        return QtUtils::toStdString(_scriptInfo.script()); 
    }

    void MongoShell::execute(const std::string &dbName)
    {
        if (_scriptInfo.execute()) {
            EventsInfo::ExecuteScriptRequestInfo inf(query(), dbName, 0, 0);
            emit startedScriptExecuted(inf);
            _server->postEventToDataBase(new Events::ExecuteScriptRequestEvent(this,inf));
            if(!_scriptInfo.script().isEmpty())
                LOG_MSG(_scriptInfo.script(), mongo::LL_INFO);
        } else {
            _scriptInfo.setScript("");
            EventsInfo::ExecuteScriptRequestInfo inf(query(), dbName, 0, 0);
            emit startedScriptExecuted(inf);
            _server->postEventToDataBase(new Events::ExecuteScriptRequestEvent(this,inf));
        }        
    }

    void MongoShell::autocomplete(const std::string &prefix)
    {
        EventsInfo::AutoCompleteRequestInfo inf(prefix);
        _server->postEventToDataBase(new Events::AutoCompleteRequestEvent(this,inf));
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
        if(type==static_cast<QEvent::Type>(Events::AutoCompleteResponceEvent::EventType)){
            Events::AutoCompleteResponceEvent *ev = static_cast<Events::AutoCompleteResponceEvent*>(event);
            Events::AutoCompleteResponceEvent::value_type v = ev->value();
            emit autoCompleteResponced(QtUtils::toQString(v._prefix), v._list);
        }
        else if(type==static_cast<QEvent::Type>(Events::ExecuteScriptResponceEvent::EventType)){
            Events::ExecuteScriptResponceEvent *ev = static_cast<Events::ExecuteScriptResponceEvent*>(event);
            Events::ExecuteScriptResponceEvent::value_type v = ev->value();
            emit finishedScriptExecuted(v);
        }

        return BaseClass::customEvent(event);
    }
}
