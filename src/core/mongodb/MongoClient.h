#ifndef MONGOCLIENT_H
#define MONGOCLIENT_H

#include <QObject>
#include <QMutex>
#include <QEvent>
#include <QStringList>
#include "events/MongoEvents.h"
#include <QScriptEngine>

namespace Robomongo
{
    class Helper;

    class MongoClient : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoClient(QString address, QObject *parent = 0);
        explicit MongoClient(QString host, int port, QObject *parent = 0);

        ~MongoClient();

        /**
         * @brief Send event to this MongoClient
         */
        void send(QEvent *event);

        /**
         * @brief Events dispatcher
         */
        virtual bool event(QEvent *event);

    private: // handlers:

        /**
         * @brief Initialize MongoClient (should be the first request)
         */
        void handle(InitRequest *event);

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

        /**
         * @brief Send reply event to object 'obj'
         */
        void reply(QObject *receiver, QEvent *event);

        QString _address;
        QThread *_thread;
        QMutex _firstConnectionMutex;

        QScriptEngine *_scriptEngine;
        Helper *_helper;

    };

    class Helper : public QObject
    {
        Q_OBJECT
    public:

        Helper() : QObject() {}
        QString text() const { return _text; }

    public slots:
        void print(const QString &message)
        {
            _text.append(message);
        }

    private:
        QString _text;
    };

}



#endif // MONGOCLIENT_H
