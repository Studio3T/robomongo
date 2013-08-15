#include "robomongo/gui/widgets/workarea/BsonWidget.h"

#include <QHBoxLayout>

#include "robomongo/gui/widgets/workarea/BsonTreeWidget.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/utils/BsonUtils.h"

namespace Robomongo
{

    BsonWidget::BsonWidget(MongoShell *shell, QWidget *parent) : QWidget(parent),
        _shell(shell),
        _bsonTree(new BsonTreeWidget(shell))
    {
        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing(0);
        hlayout->setMargin(0);
        hlayout->addWidget(_bsonTree);
        setLayout(hlayout);
    }

    void BsonWidget::setDocuments(const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo)
    {
        _bsonTree->setDocuments(documents, queryInfo);
    }

    JsonPrepareThread::JsonPrepareThread(QList<MongoDocumentPtr> bsonObjects, UUIDEncoding uuidEncoding, SupportedTimes timeZone)
        :_bsonObjects(bsonObjects),
        _uuidEncoding(uuidEncoding),
        _timeZone(timeZone),
        _stop(false)
    {
    }

    void JsonPrepareThread::stop()
    {
        _stop=true;
    }

    void JsonPrepareThread::run()
    {
        int position = 0;
        for(QList<MongoDocumentPtr>::const_iterator it = _bsonObjects.begin();it!=_bsonObjects.end();++it)
        {
            MongoDocumentPtr doc = *it;
            mongo::StringBuilder sb;
            if (position == 0)
                sb << "/* 0 */\n";
            else
                sb << "\n\n/* " << position << " */\n";

            // Approach #1
            // std::string stdJson = doc->bsonObj().jsonString(mongo::TenGen, 1);

            // Approach #2
            // std::string stdJson = doc->bsonObj().toString(false, true);

            mongo::BSONObj obj = doc->bsonObj();
            std::string stdJson = BsonUtils::jsonString(obj, mongo::TenGen, 1, _uuidEncoding, _timeZone);

            if (_stop)
                break;

            sb << stdJson;
            QString json = QString::fromUtf8(sb.str().data());

            if (_stop)
                break;

            emit partReady(json);

            position++;
        }

        emit done();
    }
}
