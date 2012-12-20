#include "Result.h"

using namespace Robomongo;

Result::Result(const QString &response, const QList<mongo::BSONObj> &documents, const QString &databaseName) :
    response(response),
    documents(documents),
    databaseName(databaseName)
{
}
