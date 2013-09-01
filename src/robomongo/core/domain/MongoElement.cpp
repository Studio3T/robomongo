#include "robomongo/core/domain/MongoElement.h"
#include <mongo/client/dbclient.h>
#include <QStringBuilder>

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/HexUtils.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/shell/db/ptimeutil.h"

using namespace mongo;

namespace Robomongo
{
    /*
    ** Create instance of MongoElement from BSONElement
    */
    MongoElement::MongoElement(BSONElement bsonElement)
    {
        _bsonElement = bsonElement;
    }

    /*
    ** String value. Only for Simple types
    */
    QString MongoElement::stringValue()
    {
        if(_stringValue.isNull())
        {
            std::string con;
            buildJsonString(con);
            _stringValue = QtUtils::toQString(con);
        }
        return _stringValue;
    }

    /*
    ** Get field name
    */
    std::string MongoElement::fieldName() const
    {
        return _bsonElement.fieldName();
    }

    /*
    ** Return MongoDocument of this element (you should check that this IS document before)
    */
    MongoDocumentPtr MongoElement::asDocument()
    {
        MongoDocument *doc = new MongoDocument(_bsonElement.Obj());
        return MongoDocumentPtr(doc);
    }

    /*
    ** Build Json string that represent this element.
    */
    void MongoElement::buildJsonString(std::string &con)
    {
       BsonUtils::buildJsonString(_bsonElement,con,AppRegistry::instance().settingsManager()->uuidEncoding(),AppRegistry::instance().settingsManager()->timeZone());
    }
}

