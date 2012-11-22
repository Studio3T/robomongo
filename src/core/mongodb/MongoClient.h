#ifndef MONGOCLIENT_H
#define MONGOCLIENT_H

#include <QObject>
#include <QMutex>

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

    signals:
        void databaseNamesLoaded(const QStringList &names);
        void connectionEstablished(const QString &address);
        void connectionFailed(const QString &address);

    private slots:
        void _establishConnection();
        void _loadDatabaseNames();

    private:

        void init();

        void invoke(char * methodName, QGenericArgument arg1 = QGenericArgument(), QGenericArgument arg2 = QGenericArgument(),
                                       QGenericArgument arg3 = QGenericArgument(), QGenericArgument arg4 = QGenericArgument(),
                                       QGenericArgument arg5 = QGenericArgument());


        QString _address;
        QThread *_thread;
        QMutex _firstConnectionMutex;

    };
}

#endif // MONGOCLIENT_H
