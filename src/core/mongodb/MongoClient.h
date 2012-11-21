#ifndef MONGOCLIENT_H
#define MONGOCLIENT_H

#include <QObject>

namespace Robomongo
{
    class MongoClient : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoClient(QString address, QObject *parent = 0);
        explicit MongoClient(QString host, int port, QObject *parent = 0);

        /**
         * @brief Load list of all database names
         */
        void loadDatabaseNames();

    signals:
        void databaseNamesLoaded(const QStringList &names);

    private slots:
        void _loadDatabaseNames();

    private:

        void init();

        void invoke(char * methodName, QGenericArgument arg1 = QGenericArgument(), QGenericArgument arg2 = QGenericArgument(),
                                       QGenericArgument arg3 = QGenericArgument(), QGenericArgument arg4 = QGenericArgument(),
                                       QGenericArgument arg5 = QGenericArgument());


        QString _address;
        QThread *_thread;

    };
}

#endif // MONGOCLIENT_H
