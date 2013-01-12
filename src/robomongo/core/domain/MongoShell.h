#pragma once

#include <QObject>

#include "robomongo/core/Core.h"
#include "robomongo/core/events/MongoEvents.h"

namespace Robomongo
{
    class EventBus;
    class MongoClient;

    class MongoShell : public QObject
    {
        Q_OBJECT

    public:
        explicit MongoShell(MongoServer *server);
        ~MongoShell();

        void open(MongoCollection *collection);
        void open(const QString &script, const QString &dbName = QString());
        void query(int resultIndex, const MongoQueryInfo &info);

        MongoServer *server() const { return _server; }
        QString query() const { return _query; }

    protected slots:
        void handle(ExecuteQueryResponse *event);
        void handle(ExecuteScriptResponse *event);

    private:
        /**
         * @brief Current query in the shell
         */
        QString _query;

        MongoServer *_server;
        MongoClient *_client;
        EventBus *_bus;
    };

}
