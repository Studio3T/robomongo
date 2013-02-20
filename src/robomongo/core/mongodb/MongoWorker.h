#pragma once

#include <QObject>
#include <QMutex>
#include <QEvent>
#include <QStringList>
#include <mongo/client/dbclient.h>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"

namespace Robomongo
{
    class Helper;
    class EventBus;
    class MongoWorkerThread;
    class MongoClient;

    class MongoWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit MongoWorker(EventBus *bus, ConnectionSettings *connection, QObject *parent = 0);

        ~MongoWorker();

        /**
         * @brief Send event to this MongoWorker
         */
        void send(Event *event);
        ScriptEngine *engine() const { return _scriptEngine; }

    protected slots: // handlers:
        /**
         * @brief Initialize MongoWorker (should be the first request)
         */
        void handle(InitRequest *event);

        /**
         * @brief Initialize MongoWorker (should be the first request)
         */
        void handle(FinalizeRequest *event);

        /**
         * @brief Initiate connection to MongoDB
         */
        void handle(EstablishConnectionRequest *event);

        /**
         * @brief Load list of all database names
         */
        void handle(LoadDatabaseNamesRequest *event);

        /**
         * @brief Load list of all collection names
         */
        void handle(LoadCollectionNamesRequest *event);

        /**
         * @brief Inserts document
         */
        void handle(InsertDocumentRequest *event);

        /**
         * @brief Remove documents
         */
        void handle(RemoveDocumentRequest *event);

        /**
         * @brief Load list of all collection names
         */
        void handle(ExecuteQueryRequest *event);

        /**
         * @brief Execute javascript
         */
        void handle(ExecuteScriptRequest *event);

        void handle(AutocompleteRequest *event);

    private:

        mongo::ScopedDbConnection *getConnection();
        MongoClient *getClient();

        /**
         * @brief Initialise MongoWorker
         */
        void init();

        /**
         * @brief Send reply event to object 'obj'
         */
        void reply(QObject *receiver, Event *event);

        QString _address;
        MongoWorkerThread *_thread;
        QMutex _firstConnectionMutex;

        ScriptEngine *_scriptEngine;
        Helper *_helper;

        bool _isAdmin;
        QString _authDatabase;

        ConnectionSettings *_connection;
        EventBus *_bus;
    };

    class Helper : public QObject
    {
        Q_OBJECT
    public:

        Helper() : QObject() {}
        QString text() const { return _text; }
        void clear() { _text = ""; }

    public slots:
        void print(const QString &message)
        {
            _text.append(message);
        }

    private:
        QString _text;
    };

}
