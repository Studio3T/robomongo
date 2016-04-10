#pragma once
#include <QObject>
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/domain/ScriptInfo.h"

namespace Robomongo
{
    class MongoShell : public QObject
    {
        Q_OBJECT

    public:
        MongoShell(MongoServer *server, const ScriptInfo &scriptInfo);

        void open(const std::string &script, const std::string &dbName = std::string());
        void query(int resultIndex, const MongoQueryInfo &info);
        void autocomplete(const std::string &prefix);
        void stop();
        MongoServer *server() const { return _server; }
        std::string query() const;
        void execute(const std::string &dbName = std::string());
        bool isExecutable() const { return _scriptInfo.execute(); }
        const QString &title() const { return _scriptInfo.title(); }
        const CursorPosition &cursor() const { return _scriptInfo.cursor(); }
        void setScript(const QString &script) { return _scriptInfo.setScript(script); }
        QString filePath() const { return _scriptInfo.filePath(); }

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
    };

}
