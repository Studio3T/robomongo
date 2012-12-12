#ifndef RESULT_H
#define RESULT_H
#include <QList>
#include "mongo/client/dbclient.h"
#include <QString>

namespace Robomongo
{
    class Result
    {
    public:
        Result(const QString &response, const QList<mongo::BSONObj> &documents);

        QList<mongo::BSONObj> documents;
        QString response;
    };
}

#endif // RESULT_H
