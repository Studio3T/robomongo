#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocumentIterator.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace mongo;

namespace
{
    QString buildSynopsis(const QString &text)
    {
        QString simplified = text.simplified().left(300);
        return simplified;
    }
}

namespace Robomongo
{
    BsonTreeItem::BsonTreeItem(MongoDocumentPtr rootDocument, MongoElementPtr element, int position) 
        : baseClass(),
        _element(element),
        _rootDocument(rootDocument),
        _position(position)
    {
        setText(0, buildFieldName());
        setForeground(2, GuiRegistry::instance().typeBrush());
        //setFlags(flags() | Qt::ItemIsEditable);

        switch (element->bsonElement().type())
        {
        /** double precision floating point value */
        case NumberDouble:
            {
                setIcon(0, GuiRegistry::instance().bsonIntegerIcon());
                setText(1, element->stringValue());
                setText(2, "Double");
            }
            break;

        /** character string, stored in utf8 */
        case String:
            {
                QString text = element->stringValue().left(500);
                setToolTip(1, text);

                setIcon(0, GuiRegistry::instance().bsonStringIcon());
                setText(1, buildSynopsis(element->stringValue()));
                setText(2, "String");
            }
            break;

        /** an embedded object */
        case Object:
            {
                setText(0, buildObjectFieldName());

                MongoDocumentPtr mongoDocument = _element->asDocument();
                setupDocument(mongoDocument);
            }
            break;

        /** an embedded array */
        case Array:
            {
                int itemsCount = _element->bsonElement().Array().size();

                setText(0, buildArrayFieldName(itemsCount));
                setIcon(0, GuiRegistry::instance().bsonArrayIcon());
                setText(2, "Array");

                setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            }
            break;

        /** binary data */
        case BinData:
            {
                setIcon(0, GuiRegistry::instance().bsonBinaryIcon());
                setText(1, element->stringValue());

                if (element->bsonElement().binDataType() == mongo::newUUID) {
                    setText(2, "UUID");
                } else if (element->bsonElement().binDataType() == mongo::bdtUUID) {

                    UUIDEncoding uuidEncoding = AppRegistry::instance().settingsManager()->uuidEncoding();
                    QString type;
                    switch(uuidEncoding) {
                    case DefaultEncoding: type = "Legacy UUID"; break;
                    case JavaLegacy:      type = "Java UUID (Legacy)"; break;
                    case CSharpLegacy:    type = ".NET UUID (Legacy)"; break;
                    case PythonLegacy:    type = "Python UUID (Legacy)"; break;
                    default:              type = "Legacy UUID"; break;
                    }

                    setText(2, type);
                } else {
                    setText(2, "Binary");
                }
            }
            break;

        /** Undefined type */
        case Undefined:
            {
                setIcon(0, GuiRegistry::instance().circleIcon());
                setText(1, "<Undefined>");
                setText(2, "Undefined");
            }
            break;

        /** ObjectId */
        case jstOID:
            {
                setIcon(0, GuiRegistry::instance().circleIcon());
                setText(1, element->stringValue());
                setText(2, "ObjectId");
            }
            break;

        /** boolean type */
        case Bool:
            {
                setIcon(0, GuiRegistry::instance().bsonBooleanIcon());
                setText(1, element->stringValue());
                setText(2, "Boolean");
            }
            break;

        /** date type */
        case Date:
            {
                setIcon(0, GuiRegistry::instance().bsonDateTimeIcon());
                setText(1, element->stringValue());
                setText(2, "Date");
            }
            break;

        /** null type */
        case jstNULL:
            {
                setIcon(0, GuiRegistry::instance().bsonNullIcon());
                setText(1, "null");
                setText(2, "Null");
            }
            break;

        /** regular expression, a pattern with options */
        case RegEx:
            {
                setIcon(0, GuiRegistry::instance().circleIcon());
                setText(1, element->stringValue());
                setText(2, "Regular Expression");
            }
            break;

        /** deprecated / will be redesigned */
        case DBRef:
            {
                setIcon(0, GuiRegistry::instance().circleIcon());
                setText(1, element->stringValue());
                setText(2, "DBRef");
            }
            break;

        /** deprecated / use CodeWScope */
        case Code:
            {
                QString code = element->stringValue();
                setToolTip(1, code);

                setIcon(0, GuiRegistry::instance().circleIcon());
                setText(1, buildSynopsis(code));
                setText(2, "Code");
            }
            break;

        /** a programming language (e.g., Python) symbol */
        case Symbol:
            {
                setText(2, "Symbol");
            }
            break;

        /** javascript code that can execute on the database server, with SavedContext */
        case CodeWScope:
            {
                QString code = element->stringValue();
                setToolTip(1, code);

                setIcon(0, GuiRegistry::instance().circleIcon());
                setText(1, buildSynopsis(code));
                setText(2, "CodeWScope");
            }
            break;

        /** 32 bit signed integer */
        case NumberInt:
            {
                setIcon(0, GuiRegistry::instance().bsonIntegerIcon());
                setText(1, element->stringValue());
                setText(2, "Int32");
            }
            break;

        /** Updated to a Date with value next OpTime on insert */
        case Timestamp:
            {
                setIcon(0, GuiRegistry::instance().bsonDateTimeIcon());
                setText(1, element->stringValue());
                setText(2, "Timestamp");
            }
            break;

        /** 64 bit integer */
        case NumberLong:
            {
                setIcon(0, GuiRegistry::instance().bsonIntegerIcon());
                setText(1, element->stringValue());
                setText(2, "Int64");
            }
            break;

        default:
            {
                setIcon(0, GuiRegistry::instance().circleIcon());
                setText(1, element->stringValue());
                setText(2, "Type is not supported");
            }
            break;
        }
    }

    BsonTreeItem::BsonTreeItem(MongoDocumentPtr document, int position) 
        : baseClass(),
        _document(document),
        _rootDocument(document),
        _position(position)
    {
        setupDocument(document);
    }

    bool BsonTreeItem::isSimpleType() const
    {
        return _element && _element->isSimpleType();
    }

    bool BsonTreeItem::isUuidType() const
    {
        return _element && _element->isUuidType();
    }

    void BsonTreeItem::setupDocument(MongoDocumentPtr document)
    {
        setText(0, buildObjectFieldName());
        setIcon(0, GuiRegistry::instance().bsonObjectIcon());

        setText(2, "Object");
        setForeground(2, GuiRegistry::instance().typeBrush());

        setExpanded(true);
        setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }

    void BsonTreeItem::expand()
    {
        QtUtils::clearChildItems(this);
        MongoDocumentPtr document = _document ? _document : _element->asDocument();
        bool isArray = _element ? _element->isArray() : false;

        MongoDocumentIterator iterator(document.get());

        int position = 0;
        while(iterator.hasMore())
        {
            MongoElementPtr element = iterator.next();
            BsonTreeItem *childItem = new BsonTreeItem(_rootDocument, element, isArray ? position : -1);
            addChild(childItem);
            position++;
        }
    }

    QString BsonTreeItem::buildFieldName()
    {
        if (_position >= 0)
            return QString("(%1)").arg(_position);
        else
            return _element->fieldName();
    }

    QString BsonTreeItem::buildObjectFieldName()
    {
        if (_position >= 0)
            return QString("(%1)  {...}").arg(_position);
        else
            return QString("%1  {...}").arg(_element->fieldName());
    }

    QString BsonTreeItem::buildArrayFieldName(int itemsCount)
    {
        if (_position >= 0)
            return QString("(%1) [%2]").arg(_position).arg(itemsCount);
        else
            return QString("%1 [%2]").arg(_element->fieldName()).arg(itemsCount);
    }
}
