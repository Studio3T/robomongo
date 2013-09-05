#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjiterator.h>
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/gui/widgets/workarea/BsonTableItem.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"

namespace
{
    using namespace Robomongo;
    void parseDocument(BsonTableItem *root, const mongo::BSONObj &doc)
    {            
            mongo::BSONObjIterator iterator(doc);
            BsonTableItem *childItem = new BsonTableItem(doc, root);
            while (iterator.more())
            {
                mongo::BSONElement element = iterator.next();
                size_t col = root->addColumn(QtUtils::toQString(std::string(element.fieldName())));
                if (BsonUtils::isArray(element)) {
                    int itemsCount = element.Array().size();
                    childItem->addRow(col,std::make_pair(QString("Array [%1]").arg(itemsCount), element));
                }
                else if (BsonUtils::isDocument(element)) {
                    parseDocument(childItem,element.Obj());
                    childItem->addRow(col, std::make_pair(QString("{%1 Keys}").arg(childItem->columnCount()), element));
                }
                else {
                    std::string result;
                    BsonUtils::buildJsonString(element, result, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone());
                    childItem->addRow(col, std::make_pair(QtUtils::toQString(result), element));
                }               
            }
            root->addChild(childItem);
    }
}

namespace Robomongo
{
    BsonTableModel::BsonTableModel(const std::vector<MongoDocumentPtr> &documents, QObject *parent) 
        : BaseClass(parent) ,
        _root(new BsonTableItem(this))
    {
        for (std::vector<MongoDocumentPtr>::const_iterator it = documents.begin(); it!=documents.end(); ++it)
        {
            MongoDocumentPtr doc = (*it);
            parseDocument(_root,doc->bsonObj());
        }
    }

    QVariant BsonTableModel::data(const QModelIndex &index, int role) const
    {
        QVariant result;

        if (!index.isValid())
            return result;

        BsonTableItem *node = _root->child(index.row());
        if (!node)
            return result;

        int col = index.column();
        mongo::BSONElement element = node->row(col).second;

        if (element.eoo()) {
            if (role == Qt::BackgroundRole) {
                return QBrush("#efedec");
            }
            return result;
        }

        if (role == Qt::DisplayRole) {
            result = node->row(col).first;
        }
        else if (role == Qt::DecorationRole) {
            switch(element.type()) {
            case mongo::NumberDouble: result = GuiRegistry::instance().bsonIntegerIcon(); break;
            case mongo::String: result = GuiRegistry::instance().bsonStringIcon(); break;
            case mongo::Object: result = GuiRegistry::instance().bsonObjectIcon(); break;
            case mongo::Array: result = GuiRegistry::instance().bsonArrayIcon(); break;
            case mongo::BinData: result = GuiRegistry::instance().bsonBinaryIcon(); break;
            case mongo::Undefined: result = GuiRegistry::instance().circleIcon(); break;
            case mongo::jstOID: result = GuiRegistry::instance().circleIcon(); break;
            case mongo::Bool: result = GuiRegistry::instance().bsonBooleanIcon(); break;
            case mongo::Date: result = GuiRegistry::instance().bsonDateTimeIcon(); break;
            case mongo::jstNULL: result = GuiRegistry::instance().bsonNullIcon(); break;
            case mongo::RegEx: result = GuiRegistry::instance().circleIcon(); break;
            case mongo::DBRef: result = GuiRegistry::instance().circleIcon(); break;
            case mongo::Code: case mongo::CodeWScope: result = GuiRegistry::instance().circleIcon(); break;
            case mongo::NumberInt: result = GuiRegistry::instance().bsonIntegerIcon(); break;
            case mongo::Timestamp: result = GuiRegistry::instance().bsonDateTimeIcon(); break;
            case mongo::NumberLong: result = GuiRegistry::instance().bsonIntegerIcon(); break;
            default: result = GuiRegistry::instance().circleIcon(); break;
            }
        }

        return result;
    }

    bool BsonTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if (index.isValid() && role == Qt::EditRole) {
            return true;
        }
        return false;
    }

    int BsonTableModel::rowCount(const QModelIndex &parent) const
    {
        return _root->childrenCount();
    }

    int BsonTableModel::columnCount(const QModelIndex &parent) const
    {
        return _root->columnCount(); 
    }

    QVariant BsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if(role != Qt::DisplayRole)
            return QVariant();

        if(orientation == Qt::Horizontal && role == Qt::DisplayRole){
            return _root->column(section); 
        }else{
            return QString("%1").arg( section + 1 ); 
        }
    }

    Qt::ItemFlags BsonTableModel::flags(const QModelIndex &index) const
    {
        if (!index.isValid())
            return Qt::ItemIsEnabled;

        return BaseClass::flags(index) ;
    }

    QModelIndex BsonTableModel::index(int row, int column, const QModelIndex &parent) const
    {
        QModelIndex index;	
        if(hasIndex(row, column, parent))
        {
            const BsonTableItem * parentItem=NULL;
            if (!parent.isValid())
            {
                parentItem = _root;
            }
            else
            {
                parentItem = QtUtils::item<BsonTableItem*>(parent);
            }
            BsonTableItem *childItem = parentItem->child(row);
            if (childItem)
            {
                index = createIndex(row, column, childItem);
            }
        }
        return index;
    }
}
