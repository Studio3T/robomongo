#ifndef MONGOCLIENT_H
#define MONGOCLIENT_H

#include <QObject>
#include <QMutex>
#include <QEvent>
#include <QStringList>

namespace Robomongo
{
    class MongoClient : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoClient(QString address, QObject *parent = 0);
        explicit MongoClient(QString host, int port, QObject *parent = 0);

        ~MongoClient();

        void establishConnection();

        /**
         * @brief Load list of all database names
         */
        void loadDatabaseNames();

        /**
         * @brief Load list of all collection names
         */
        void loadCollectionNames(QObject *sender, const QString &databaseName);

    signals:
        void databaseNamesLoaded(const QStringList &names);
        void collectionNamesLoaded(const QString &databaseName, const QStringList &names);
        void connectionEstablished(const QString &address);
        void connectionFailed(const QString &address);

    private slots:
        void _establishConnection();
        void _loadDatabaseNames();
        void _loadCollectionNames(QObject *sender, const QString &databaseName);

    private:

        void init();

        void invoke(char * methodName, QGenericArgument arg1 = QGenericArgument(), QGenericArgument arg2 = QGenericArgument(),
                                       QGenericArgument arg3 = QGenericArgument(), QGenericArgument arg4 = QGenericArgument(),
                                       QGenericArgument arg5 = QGenericArgument());


        QString _address;
        QThread *_thread;
        QMutex _firstConnectionMutex;

    };

    class CollectionNamesLoaded : public QEvent
    {
    public:

        CollectionNamesLoaded(const QString &databaseName, const QStringList &collectionNames)
            : _databaseName(databaseName), _collectionNames(collectionNames), QEvent((QEvent::Type)(QEvent::User + 1)) { }

        QString _databaseName;
        QStringList _collectionNames;
    };
}

#endif // MONGOCLIENT_H
