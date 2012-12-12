#include "Result.h"

using namespace Robomongo;

Result::Result(const QString &response, const QList<mongo::BSONObj> &documents) :
    response(response),
    documents(documents)
{
}
