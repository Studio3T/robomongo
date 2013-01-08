#include "robomongo/core/engine/Result.h"

using namespace Robomongo;

Result::Result(const QString &response, const QList<mongo::BSONObj> &documents,
               const QueryInfo &queryInfo, qint64 elapsedms) :
    response(response),
    documents(documents),
    queryInfo(queryInfo),
    elapsedms(elapsedms) { }
