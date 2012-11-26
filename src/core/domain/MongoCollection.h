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
        MongoCollection(const MongoDatabase *database, const QString &name);

        bool isSystem() const { return _system; }
        QString name() const { return _name; }

    signals:

    public slots:

    private:
        const MongoDatabase *_database;
        QString _name;
        bool _system;


    };
}



#endif // MONGOCOLLECTION_H
