#pragma once

#include <QObject>
#include <QMutex>
#include <QEvent>
#include <QStringList>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/engine/ScriptEngine.h"

namespace Robomongo
{
    class Helper;
    class EventBus;
    class MongoClientThread;

    class MongoClient : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoClient(EventBus *bus, ConnectionSettings *connection, QObject *parent = 0);

        ~MongoClient();

        /**
         * @brief Send event to this MongoClient
         */
        void send(Event *event);
        ScriptEngine *engine() const { return _scriptEngine; }

    protected slots: // handlers:

        /**
         * @brief Initialize MongoClient (should be the first request)
         */
        void handle(InitRequest *event);

        /**
         * @brief Initialize MongoClient (should be the first request)
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
         * @brief Load list of all collection names
         */
        void handle(ExecuteQueryRequest *event);

        /**
         * @brief Execute javascript
         */
        void handle(ExecuteScriptRequest *event);

    private:

        /**
         * @brief Initialise MongoClient
         */
        void init();

        void evaluteFile(const QString &path);

        /**
         * @brief Send reply event to object 'obj'
         */
        void reply(QObject *receiver, Event *event);

        QString _address;
        MongoClientThread *_thread;
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
