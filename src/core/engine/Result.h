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
        Result(const QString &response, const QList<mongo::BSONObj> &documents,
               const QString &serverName, bool isServerValid,
               const QString &databaseName, bool isDatabaseValid,
               const QString &collectionName, bool isCollectionValid);

        QList<mongo::BSONObj> documents;
        QString response;
        QString serverName;
        bool isServerValid;
        QString databaseName;
        bool isDatabaseValid;
        QString collectionName;
        bool isCollectionValid;
    };

    class ExecResult
    {
    public:
        ExecResult() {}
        ExecResult(const QList<Result> &results,
                   const QString &currentServerName, bool isCurrentServerValid,
                   const QString &currentDatabaseName, bool isCurrentDatabaseValid) :
            results(results),
            currentServerName(currentServerName),
            isCurrentServerValid(isCurrentServerValid),
            currentDatabaseName(currentDatabaseName),
            isCurrentDatabaseValid(isCurrentDatabaseValid){}

        QList<Result> results;
        QString currentServerName;
        bool isCurrentServerValid;
        QString currentDatabaseName;
        bool isCurrentDatabaseValid;
    };
}

#endif // RESULT_H
