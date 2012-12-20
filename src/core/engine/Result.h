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
        Result(const QString &response, const QList<mongo::BSONObj> &documents, const QString &databaseName);

        QList<mongo::BSONObj> documents;
        QString response;
        QString databaseName;
    };
}

#endif // RESULT_H
