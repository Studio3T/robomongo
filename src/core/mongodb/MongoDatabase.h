#ifndef MONGODATABASE_H
#define MONGODATABASE_H

#include <QObject>
#include "Core.h"
#include "mongo/client/dbclient.h"
#include "boost/shared_ptr.hpp"

namespace Robomongo
{
    class MongoDatabase : public QObject, public boost::enable_shared_from_this<MongoDatabase>
    {
        Q_OBJECT

    public:

        /**
         * @brief MongoDatabase
         * @param server - pointer to parent MongoServer
         */
        MongoDatabase(const MongoServer *server, const QString &name);
        ~MongoDatabase();

        void listCollections();

        QString name() const { return _name; }

        /**
         * @brief Checks that this is a system database
         * @return true if system, false otherwise
         */
        bool isSystem() const { return _system; }

        virtual bool event(QEvent *);


    signals:

        void collectionListLoaded(const QList<MongoCollectionPtr> &list);

    private slots:
        void collectionNamesLoaded(const QString &databaseName, const QStringList &collectionNames);

    private:

        const MongoServer *_server;
        QString _name;
        bool _system;

    };

}

#endif // MONGODATABASE_H
