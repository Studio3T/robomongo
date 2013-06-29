#include "robomongo/core/domain/MongoFunction.h"
#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjbuilder.h>
namespace Robomongo
{
    MongoFunction::MongoFunction(const mongo::BSONObj &obj)
    {
        _name = QString::fromStdString(obj.getField("_id").String());
        _code = QString::fromUtf8(obj.getField("value")._asCode().data());
    }

    mongo::BSONObj MongoFunction::toBson() const
    {
        mongo::BSONObjBuilder builder;

        QByteArray bytes = _code.toUtf8();

        mongo::BSONCode code = mongo::BSONCode(bytes.constData());
        builder.append("_id", _name.toStdString());
        builder.append("value", code);
        mongo::BSONObj obj = builder.obj();
        return obj;
    }
}
