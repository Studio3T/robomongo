#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocumentIterator.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"

using namespace Robomongo;
using namespace mongo;

BsonTreeItem::BsonTreeItem(MongoDocumentPtr rootDocument, MongoElementPtr element, int position) : QObject(),
    _element(element),
    _rootDocument(rootDocument),
    _settingsManager(AppRegistry::instance().settingsManager())
{
	_position = position;

	setText(0, buildFieldName());
    setForeground(2, GuiRegistry::instance().typeBrush());
    //setFlags(flags() | Qt::ItemIsEditable);

	switch (element->bsonElement().type())
    {
	/** double precision floating point value */
	case NumberDouble:
        {
            static QString typeDouble("Double");
            setIcon(0, GuiRegistry::instance().bsonIntegerIcon());
            setText(1, element->stringValue());
            setText(2, typeDouble);
        }
		break;

	/** character string, stored in utf8 */
	case String:
		{
            static QString typeString("String");
			QString text = element->stringValue().left(500);
			setToolTip(1, text);

            setIcon(0, GuiRegistry::instance().bsonStringIcon());
			setText(1, buildSynopsis(element->stringValue()));
            setText(2, typeString);
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
            static QString typeArray("Array");
			int itemsCount = _element->bsonElement().Array().size();

			setText(0, buildArrayFieldName(itemsCount));
            setIcon(0, GuiRegistry::instance().bsonArrayIcon());
            setText(2, typeArray);

			setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
		}		
		break;

	/** binary data */
	case BinData:
        {
            static QString typeBinary("Binary");
            setIcon(0, GuiRegistry::instance().bsonBinaryIcon());
            setText(1, element->stringValue());

            if (element->bsonElement().binDataType() == mongo::newUUID) {
                setText(2, "UUID");
            } else if (element->bsonElement().binDataType() == mongo::bdtUUID) {

                UUIDEncoding uuidEncoding = _settingsManager->uuidEncoding();
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
                setText(2, typeBinary);
            }
        }
		break;

	/** Undefined type */
	case Undefined:
        {
            static QString typeUndefined("Undefined");
            setIcon(0, GuiRegistry::instance().circleIcon());
            setText(1, "<Undefined>");
            setText(2, typeUndefined);
        }
		break;

	/** ObjectId */
	case jstOID: 
        {
            static QString typeObjectId("ObjectId");
            setIcon(0, GuiRegistry::instance().circleIcon());
            setText(1, element->stringValue());
            setText(2, typeObjectId);
        }
		break;

	/** boolean type */
	case Bool:
        {
            static QString typeBoolean("Boolean");
            setIcon(0, GuiRegistry::instance().bsonBooleanIcon());
            setText(1, element->stringValue());
            setText(2, typeBoolean);
        }
		break;

	/** date type */
	case Date:
        {
            static QString typeDate("Date");
            setIcon(0, GuiRegistry::instance().bsonDateTimeIcon());
            setText(1, element->stringValue());
            setText(2, typeDate);
        }
		break;

	/** null type */
	case jstNULL:
        {
            static QString typeNull("Null");
            setIcon(0, GuiRegistry::instance().bsonNullIcon());
            setText(1, "null");
            setText(2, typeNull);
        }
		break;

	/** regular expression, a pattern with options */
	case RegEx:
        {
            static QString typeRegExp("Regular Expression");
            setIcon(0, GuiRegistry::instance().circleIcon());
            setText(1, element->stringValue());
            setText(2, typeRegExp);
        }
        break;

	/** deprecated / will be redesigned */
	case DBRef:
        {
            static QString typeDBRef("DBRef");
            setIcon(0, GuiRegistry::instance().circleIcon());
            setText(1, element->stringValue());
            setText(2, typeDBRef);
        }
		break;

	/** deprecated / use CodeWScope */
	case Code:
        {
            static QString typeCode("Code");

            QString code = element->stringValue();
            setToolTip(1, code);

            setIcon(0, GuiRegistry::instance().circleIcon());
            setText(1, buildSynopsis(code));
            setText(2, typeCode);
        }
		break;

	/** a programming language (e.g., Python) symbol */
	case Symbol:
        {
            static QString typeSymbol("Symbol");
            setText(2, typeSymbol);
        }
		break;

	/** javascript code that can execute on the database server, with SavedContext */
	case CodeWScope:
        {
            static QString typeCodeWScope("CodeWScope");

            QString code = element->stringValue();
            setToolTip(1, code);

            setIcon(0, GuiRegistry::instance().circleIcon());
            setText(1, buildSynopsis(code));
            setText(2, typeCodeWScope);
        }
		break;

	/** 32 bit signed integer */
	case NumberInt:
        {
            static QString typeInteger("Int32");
            setIcon(0, GuiRegistry::instance().bsonIntegerIcon());
            setText(1, element->stringValue());
            setText(2, typeInteger);
        }
		break;

	/** Updated to a Date with value next OpTime on insert */
	case Timestamp:
        {
            static QString typeTimestamp("Timestamp");
            setIcon(0, GuiRegistry::instance().bsonDateTimeIcon());
            setText(1, element->stringValue());
            setText(2, typeTimestamp);
        }
		break;

	/** 64 bit integer */
	case NumberLong:
        {
            static QString typeLong("Int64");
            setIcon(0, GuiRegistry::instance().bsonIntegerIcon());
            setText(1, element->stringValue());
            setText(2, typeLong);
        }
		break; 

	default:
        {
            static QString typeLong("Type is not supported");
            setIcon(0, GuiRegistry::instance().circleIcon());
            setText(1, element->stringValue());
            setText(2, typeLong);
        }
		break;
	}
}

BsonTreeItem::BsonTreeItem(MongoDocumentPtr document, int position) : QObject(),
    _document(document),
    _rootDocument(document)
{
	_position = position;
    setupDocument(document);
}

bool BsonTreeItem::isSimpleType()
{
    return _element && _element->isSimpleType();
}

bool BsonTreeItem::isUuidType()
{
    return _element && _element->isUuidType();
}

void BsonTreeItem::setupDocument(MongoDocumentPtr document)
{
	setText(0, buildObjectFieldName());
    setIcon(0, GuiRegistry::instance().bsonObjectIcon());

    static QString typeObject("Object");
    setText(2, typeObject);
    setForeground(2, GuiRegistry::instance().typeBrush());

	setExpanded(true);
	setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}

void BsonTreeItem::expand()
{
	cleanChildItems();

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

void BsonTreeItem::cleanChildItems()
{
	int itemCount = childCount();
	for (int i = 0; i < itemCount; ++i)
	{
        QTreeWidgetItem *p = child(0);
		removeChild(p);
		delete p;
	}
}

QString BsonTreeItem::buildFieldName()
{
	QString fieldName;

	if (_position >= 0)
		return QString("(%1)").arg(_position); 
	else
		return _element->fieldName();
}

QString BsonTreeItem::buildObjectFieldName()
{
	QString fieldName;

	if (_position >= 0)
		return QString("(%1)  {...}").arg(_position); 
	else
		return QString("%1  {...}").arg(_element->fieldName());
}

QString BsonTreeItem::buildArrayFieldName(int itemsCount)
{
	QString fieldName;

	if (_position >= 0)
		return QString("(%1) [%2]").arg(_position).arg(itemsCount); 
	else
		return QString("%1 [%2]").arg(_element->fieldName()).arg(itemsCount);
}

QString BsonTreeItem::buildSynopsis(QString text)
{
	QString simplified = text.simplified().left(300);
	return simplified;
}
