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

        bool event(QEvent *event);

        MongoServerWeakPtr server() const { return _server; }
        QString query() const { return _query; }


    private: //handlers
        void handle(const ExecuteQueryResponse *event);
        void handle(const ExecuteScriptResponse *event);

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
