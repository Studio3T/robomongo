#ifndef MONGOSHELL_H
#define MONGOSHELL_H

#include <QObject>
#include "Core.h"
#include "mongodb/MongoClient.h"

namespace Robomongo
{
    class Dispatcher;

    class MongoShell : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoShell(MongoServer *server);

        void open(const MongoCollectionPtr &collection);
        void open(const QString &script);

        bool event(QEvent *event);

    private: //handlers
        void handle(const ExecuteQueryResponse *event);
        void handle(const ExecuteScriptResponse *event);

    private:

        /**
         * @brief Current query in the shell
         */
        QString _query;

        MongoServer *_server;

        MongoClient *_client;

        Dispatcher *_dispatcher;
    };
}

#endif // MONGOSHELL_H
