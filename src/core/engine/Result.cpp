#include "Result.h"

using namespace Robomongo;

Result::Result(const QString &response, const QList<mongo::BSONObj> &documents, const QString &databaseName, bool isDatabaseValid) :
    response(response),
    documents(documents),
    databaseName(databaseName),
    isDatabaseValid(isDatabaseValid) { }
