#ifndef MONGOCOLLECTION_H
#define MONGOCOLLECTION_H

#include <QObject>
#include "Core.h"
#include "MongoDatabase.h"

namespace Robomongo
{
    class MongoCollection : public QObject, public boost::enable_shared_from_this<MongoCollection>
    {
        Q_OBJECT
    public:
        MongoCollection(MongoDatabase *database, const QString &name);

        bool isSystem() const { return _system; }

        QString name() const { return _name; }

    signals:

    public slots:

    private:

        /**
         * @brief Database that contains this collection
         */
        MongoDatabase *_database;

        /*
        ** Name of collection (without database prefix)
        */
        QString _name;

        /*
        ** Full name of collection (with database prefix)
        */
        QString _fullName;


        bool _system;


    };
}



#endif // MONGOCOLLECTION_H
