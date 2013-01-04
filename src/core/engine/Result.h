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
        Result(const QString &response, const QList<mongo::BSONObj> &documents, const QString &databaseName, bool isDatabaseValid);

        QList<mongo::BSONObj> documents;
        QString response;
        QString databaseName;
        bool isDatabaseValid;
    };

    class ExecResult
    {
    public:
        ExecResult() {}
        ExecResult(const QList<Result> &results, const QString &currentDatabaseName, bool isCurrentDatabaseValid) :
            results(results),
            currentDatabaseName(currentDatabaseName),
            isCurrentDatabaseValid(isCurrentDatabaseValid){}

        QList<Result> results;
        QString currentDatabaseName;
        bool isCurrentDatabaseValid;
    };
}

#endif // RESULT_H
