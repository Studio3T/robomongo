#ifndef MONGOSHELL_H
#define MONGOSHELL_H

#include <QObject>

namespace Robomongo
{
    class MongoShell : public QObject
    {
        Q_OBJECT
    public:
        explicit MongoShell(QObject *parent = 0);

    };
}

#endif // MONGOSHELL_H
