#include "Result.h"

using namespace Robomongo;

Result::Result(const QString &response, const QList<mongo::BSONObj> &documents,
               const QueryInfo &queryInfo) :
    response(response),
    documents(documents),
    queryInfo(queryInfo) { }
