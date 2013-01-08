#ifndef RESULT_H
#define RESULT_H

#include <QList>
#include <QString>
#include "mongo/client/dbclient.h"

namespace Robomongo
{
    class QueryInfo
    {
    public:

        QueryInfo() :
            isNull(true) {}

        QueryInfo(const QString &server, const QString &database, const QString &collection,
                  mongo::BSONObj query, mongo::BSONObj fields, int limit, int skip, int batchSize,
                  int options, bool special) :
            serverAddress(server),
            databaseName(database),
            collectionName(collection),
            query(query),
            fields(fields),
            limit(limit),
            skip(skip),
            batchSize(batchSize),
            options(options),
            special(special),
            isNull(false) {}

        QString serverAddress;
        QString databaseName;
        QString collectionName;
        mongo::BSONObj query;
        mongo::BSONObj fields;
        int limit;
        int skip;
        int batchSize;
        int options;
        bool special; // flag, indicating that `query` contains special fields on
                      // first level, and query data in `query` field.
        bool isNull;
    };

    class Result
    {
    public:
        Result(const QString &response,
               const QList<mongo::BSONObj> &documents,
               const QueryInfo &queryInfo,
               qint64 elapsedms);

        QString response;
        QList<mongo::BSONObj> documents;
        QueryInfo queryInfo;
        qint64 elapsedms;
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
