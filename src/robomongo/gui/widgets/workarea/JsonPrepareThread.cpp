#include "robomongo/gui/widgets/workarea/JsonPrepareThread.h"

#include <QHBoxLayout>

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    JsonPrepareThread::JsonPrepareThread(const std::vector<MongoDocumentPtr> &bsonObjects, UUIDEncoding uuidEncoding, SupportedTimes timeZone)
        :_bsonObjects(bsonObjects),
        _uuidEncoding(uuidEncoding),
        _timeZone(timeZone),
        _stop(false)
    {
    }

    void JsonPrepareThread::stop()
    {
        _stop = true;
    }

    void JsonPrepareThread::run()
    {
        int position = 1; // 1-based numbering to match tree & table views
        for (std::vector<MongoDocumentPtr>::const_iterator it = _bsonObjects.begin(); it != _bsonObjects.end(); ++it)
        {
            MongoDocumentPtr doc = *it;
            mongo::StringBuilder sb;
            if (position == 1)
                sb << "/* 1 */\n";
            else
                sb << "\n\n/* " << position << " */\n";

            mongo::BSONObj obj = doc->bsonObj();
            std::string stdJson = BsonUtils::jsonString(obj, mongo::TenGen, 1, _uuidEncoding, _timeZone);

            if (_stop)
                break;

            sb << stdJson;
            QString json = QtUtils::toQString(sb.str());

            if (_stop)
                break;

            emit partReady(json);

            position++;
        }

        emit done();
    }
}
