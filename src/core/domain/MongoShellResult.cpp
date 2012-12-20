#include "MongoShellResult.h"
#include "domain/MongoDocument.h"

using namespace Robomongo;

MongoShellResult::MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents, const QString &databaseName) :
    response(response),
    documents(documents),
    databaseName(databaseName)
{

}

QList<MongoShellResult> MongoShellResult::fromResult(QList<Result> results)
{
    QList<MongoShellResult> shellResults;

    foreach(Result result, results) {
        QList<MongoDocumentPtr> list;
        foreach(mongo::BSONObj obj, result.documents) {
            MongoDocumentPtr doc(new MongoDocument(obj));
            list.append(doc);
        }

        shellResults.append(MongoShellResult(result.response, list, result.databaseName));
    }

    return shellResults;
}

