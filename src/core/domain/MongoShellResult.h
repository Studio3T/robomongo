#ifndef MONGOSHELLRESULT_H
#define MONGOSHELLRESULT_H

#include <engine/Result.h>
#include "Core.h"

namespace Robomongo
{
    class MongoShellResult
    {
    public:
        MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents, const QString &databaseName);
        QList<MongoDocumentPtr> documents;
        QString response;
        QString databaseName;

        static QList<MongoShellResult> fromResult(QList<Result> result);
    };
}

#endif // MONGOSHELLRESULT_H
