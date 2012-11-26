#ifndef MONGOSHELL_H
#define MONGOSHELL_H

#include <QObject>
#include "Core.h"

namespace Robomongo
{
    class MongoShell : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoShell(QObject *parent = 0);

        void open(const MongoCollectionPtr &collection);
    };
}

#endif // MONGOSHELL_H
