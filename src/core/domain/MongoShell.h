#ifndef MONGOSHELL_H
#define MONGOSHELL_H

#include <QObject>
#include "Core.h"
#include "events/MongoEvents.h"

namespace Robomongo
{
    class Dispatcher;
    class MongoClient;

    class MongoShell : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoShell(const MongoServerPtr server);
        ~MongoShell();

        void open(const MongoCollectionPtr &collection);
        void open(const QString &script, const QString &dbName = QString());

        MongoServerWeakPtr server() const { return _server; }
        QString query() const { return _query; }


    protected slots:
        void handle(ExecuteQueryResponse *event);
        void handle(ExecuteScriptResponse *event);

    private:

        /**
         * @brief Current query in the shell
         */
        QString _query;

        MongoServerWeakPtr _server;

        MongoClient *_client;

        Dispatcher *_dispatcher;
    };

}

#endif // MONGOSHELL_H
