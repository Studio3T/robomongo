#pragma once
#include <QObject>
#include "robomongo/core/domain/ScriptInfo.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/events/MongoEvents.hpp"

namespace Robomongo
{
    class MongoServer;
    class MongoShell : public QObject
    {
        Q_OBJECT

    public:
        typedef QObject BaseClass;
        MongoShell(MongoServer *server,const ScriptInfo &scriptInfo);

        void open(const std::string &script, const std::string &dbName = std::string());
        void autocomplete(const std::string &prefix);
        void stop();
        MongoServer *server() const { return _server; }
        std::string query() const;
        void execute(const std::string &dbName = std::string());
        bool isExecutable() const {return _scriptInfo.execute(); }
        const QString &title() const { return _scriptInfo.title(); }
        const CursorPosition &cursor() const { return _scriptInfo.cursor(); }
        void setScript(const QString &script) { return _scriptInfo.setScript(script); }
        QString filePath() const { return _scriptInfo.filePath(); }

        bool saveToFile();
        bool saveToFileAs();
        bool loadFromFile();

        virtual void customEvent(QEvent *);

    Q_SIGNALS:
        void autoCompleteResponced(const QString &prefix,const QStringList &list);
        void scriptExecuted(const EventsInfo::ExecuteScriptResponceInfo &inf);
        void startScriptExecuted();

    private:        
        ScriptInfo _scriptInfo;
        MongoServer *_server;
    };

}
