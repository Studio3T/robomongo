#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoDocument.h"

using namespace Robomongo;

MongoShellResult::MongoShellResult(const QString &response, const QList<MongoDocumentPtr> &documents, const QueryInfo &queryInfo, qint64 elapsedms) :
    response(response),
    documents(documents),
    queryInfo(queryInfo),
    elapsedms(elapsedms) { }

QList<MongoShellResult> MongoShellResult::fromResult(QList<Result> results)
{
    QList<MongoShellResult> shellResults;

    foreach(Result result, results) {
        QList<MongoDocumentPtr> list;
        foreach(mongo::BSONObj obj, result.documents) {
            MongoDocumentPtr doc(new MongoDocument(obj));
            list.append(doc);
        }

        shellResults.append(MongoShellResult(result.response, list, result.queryInfo, result.elapsedms));
    }

    return shellResults;
}

