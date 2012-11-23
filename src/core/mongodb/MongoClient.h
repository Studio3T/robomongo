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

        void establishConnection(QObject *sender);

        /**
         * @brief Load list of all database names
         */
        void loadDatabaseNames(QObject *sender);

        /**
         * @brief Load list of all collection names
         */
        void loadCollectionNames(QObject *sender, const QString &databaseName);

    private slots:
        void _establishConnection(QObject *sender);
        void _loadDatabaseNames(QObject *sender);
        void _loadCollectionNames(QObject *sender, const QString &databaseName);

    private:

        void init();

        void reply(QObject *obj, QEvent *event);
        void invoke(char *methodName, QGenericArgument arg1 = QGenericArgument(), QGenericArgument arg2 = QGenericArgument(),
                                      QGenericArgument arg3 = QGenericArgument(), QGenericArgument arg4 = QGenericArgument(),
                                      QGenericArgument arg5 = QGenericArgument());


        QString _address;
        QThread *_thread;
        QMutex _firstConnectionMutex;

    };
}

#endif // MONGOCLIENT_H
