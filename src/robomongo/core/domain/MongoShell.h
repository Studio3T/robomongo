#pragma once
#include <QObject>
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/domain/ScriptInfo.h"

namespace Robomongo
{
    class EventBus;
    class MongoWorker;
    class MongoShell : public QObject
    {
        Q_OBJECT

    public:
        MongoShell(MongoServer *server,const ScriptInfo &scriptInfo);
        ~MongoShell();

        void open(const QString &script, const QString &dbName = QString());
        void query(int resultIndex, const MongoQueryInfo &info);
        void autocomplete(const QString &prefix);
        void stop();
        MongoServer *server() const { return _server; }
        QString query() const { return _scriptInfo.script(); }
        void execute(const QString &dbName = QString());
        bool isExecutable() const {return _scriptInfo.execute(); }
        const QString &title() const { return _scriptInfo.title(); }
        const CursorPosition &cursor() const { return _scriptInfo.cursor(); }
        void setScript(const QString &script){return _scriptInfo.setScript(script);}
        QString filePath() const
        {
            return _scriptInfo.filePath();
        }

        bool saveToFile();
        bool saveToFileAs();
        bool loadFromFile();

    protected Q_SLOTS:
        void handle(ExecuteQueryResponse *event);
        void handle(ExecuteScriptResponse *event);
        void handle(AutocompleteResponse *event);

    private:        
        ScriptInfo _scriptInfo;
        MongoServer *_server;
        MongoWorker *_client;
        EventBus *_bus;
    };

}
