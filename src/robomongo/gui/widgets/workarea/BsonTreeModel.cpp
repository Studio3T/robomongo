#include "robomongo/gui/widgets/workarea/BsonTreeModel.h"

#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjiterator.h>
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"

namespace
{
    using namespace Robomongo;
    void parseDocument(BsonTreeItem *root, const mongo::BSONObj &doc)
    {            
            mongo::BSONObjIterator iterator(doc);
            while (iterator.more())
            {
                mongo::BSONElement element = iterator.next();                
                BsonTreeItem *childItemInner = new BsonTreeItem(doc, root);
                childItemInner->setKey(QtUtils::toQString(std::string(element.fieldName())));

                if (BsonUtils::isArray(element)) {
                    int itemsCount = element.Array().size();
                    childItemInner->setValue(QString("Array [%1]").arg(itemsCount));
                    parseDocument(childItemInner,element.Obj());  
                }
                else if (BsonUtils::isDocument(element)) {
                    parseDocument(childItemInner,element.Obj());                    
                }
                else {
                    std::string result;
                    BsonUtils::buildJsonString(element, result, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone());
                    childItemInner->setValue(QtUtils::toQString(result));
                }
                childItemInner->setType(element.type());
                if(element.type()==mongo::BinData){
                    childItemInner->setBinType(element.binDataType());
                }
                root->addChild(childItemInner);
                root->setValue(QString("{%1 Keys}").arg(root->childrenCount()));
            }            
    }
}

namespace Robomongo
{
    BsonTreeModel::BsonTreeModel(const std::vector<MongoDocumentPtr> &documents, QObject *parent) 
        : BaseClass(parent) ,
        _root(new BsonTreeItem(this))
    {
        for (int i = 0; i<documents.size(); ++i)
        {
            MongoDocumentPtr doc = documents[i]; 
            BsonTreeItem *child = new BsonTreeItem(doc->bsonObj(), _root);
            parseDocument(child,doc->bsonObj());
            child->setKey(QString("(%1) {...}").arg(i));
            child->setType(mongo::Object);
            _root->addChild(child);
        }
    }

    const QIcon &BsonTreeModel::getIcon(BsonTreeItem *item)
    {
        switch(item->type()) {
        case mongo::NumberDouble: return GuiRegistry::instance().bsonIntegerIcon();
        case mongo::String: return GuiRegistry::instance().bsonStringIcon();
        case mongo::Object: return GuiRegistry::instance().bsonObjectIcon();
        case mongo::Array: return GuiRegistry::instance().bsonArrayIcon();
        case mongo::BinData: return GuiRegistry::instance().bsonBinaryIcon();
        case mongo::Undefined: return GuiRegistry::instance().circleIcon();
        case mongo::jstOID: return GuiRegistry::instance().circleIcon();
        case mongo::Bool: return GuiRegistry::instance().bsonBooleanIcon();
        case mongo::Date: return GuiRegistry::instance().bsonDateTimeIcon();
        case mongo::jstNULL: return GuiRegistry::instance().bsonNullIcon();
        case mongo::RegEx: return GuiRegistry::instance().circleIcon();
        case mongo::DBRef: return GuiRegistry::instance().circleIcon();
        case mongo::Code: case mongo::CodeWScope: return GuiRegistry::instance().circleIcon();
        case mongo::NumberInt: return GuiRegistry::instance().bsonIntegerIcon();
        case mongo::Timestamp: return GuiRegistry::instance().bsonDateTimeIcon();
        case mongo::NumberLong: return GuiRegistry::instance().bsonIntegerIcon();
        default: return GuiRegistry::instance().circleIcon();
        }
    }

    QVariant BsonTreeModel::data(const QModelIndex &index, int role) const
    {
        QVariant result;

        if (!index.isValid())
            return result;

        BsonTreeItem *node = QtUtils::item<BsonTreeItem*>(index);

        if (!node)
            return result;

        int col = index.column();

        if (role == Qt::DisplayRole||Qt::DecorationRole) {
            if (col==BsonTreeItem::eKey){
                if(role == Qt::DisplayRole){
                    result = node->key();
                }
                else if (role == Qt::DecorationRole) {
                    return getIcon(node);
                }
            }
            else if(col == BsonTreeItem::eValue && role == Qt::DisplayRole){
                result = node->value();
            }
            else if(col == BsonTreeItem::eType && role == Qt::DisplayRole){
                result = BsonUtils::BSONTypeToString(node->type(),node->binType(),AppRegistry::instance().settingsManager()->uuidEncoding());
            }
        }        

        return result;
    }

    bool BsonTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        if(!index.isValid())
            return false;
        if (role != Qt::EditRole)
            return false;
        if(index.column()!=BsonTreeItem::eValue)
            return false;
        
        BsonTreeItem *it = QtUtils::item<BsonTreeItem*>(index);
        QString val =value.toString();
        bool result=false;
        if(!val.isEmpty()&&val!=it->value()){
            result = true;
            it->setValue(val);
        }

        if (result)
            emit dataChanged(index, index);

        return result;
    }

    Qt::ItemFlags BsonTreeModel::flags(const QModelIndex &index) const
    {
        Qt::ItemFlags result = 0;
        if (index.isValid()){
            result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            if(index.column()==BsonTreeItem::eValue)
                result |= Qt::ItemIsEditable;
        }
        return result;
    }

    int BsonTreeModel::rowCount(const QModelIndex &parent) const
    {
        const BsonTreeItem *parentItem=NULL;
        if (parent.isValid())
            parentItem = QtUtils::item<BsonTreeItem*>(parent);
        else
            parentItem = _root;

        return parentItem->childrenCount();
    }

    int BsonTreeModel::columnCount(const QModelIndex &parent) const
    {
        return BsonTreeItem::eCountColumns;
    }

    QVariant BsonTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if(role != Qt::DisplayRole)
            return QVariant();

        if(orientation == Qt::Horizontal && role == Qt::DisplayRole){
            if(section==BsonTreeItem::eKey){
                return "Key";
            }
            else if(section==BsonTreeItem::eValue){
                return "Value";
            }
            else{
                return "Type";
            }
        }

        return BaseClass::headerData(section,orientation,role);
    }

    QModelIndex BsonTreeModel::parent(const QModelIndex& index) const
    {	
        QModelIndex result;
        if (index.isValid())
        {
            BsonTreeItem *const childItem = QtUtils::item<BsonTreeItem*const>(index);
            BsonTreeItem *const parentItem = static_cast<BsonTreeItem*const>(childItem->parent());
            if (parentItem&&parentItem!=_root)
            {
                BsonTreeItem *const grandParent = static_cast<BsonTreeItem*const>(parentItem->parent());
                int row = grandParent->indexOf(parentItem);
                result= createIndex(row, 0, parentItem);
            }
        }
        return result;
    }

    QModelIndex BsonTreeModel::index(int row, int column, const QModelIndex &parent) const
    {
        QModelIndex index;	
        if(hasIndex(row, column, parent))
        {
            const BsonTreeItem * parentItem=NULL;
            if (!parent.isValid())
            {
                parentItem = _root;
            }
            else
            {
                parentItem = QtUtils::item<BsonTreeItem*>(parent);
            }

            BsonTreeItem *childItem = parentItem->child(row);
            if (childItem)
            {
                index = createIndex(row, column, childItem);
            }
        }
        return index;
    }

    void BsonTreeModel::insertItem(BsonTreeItem *parent, BsonTreeItem *children)
    {
        QModelIndex index = createIndex(0,0,parent);
        unsigned child_count = parent->childrenCount();
        beginInsertRows(index,child_count,child_count);
        parent->addChild(children);
        endInsertRows();
    }

    void BsonTreeModel::removeitem(BsonTreeItem *children)
    {
        BsonTreeItem *parent = static_cast<BsonTreeItem *>(children->parent());
        if(parent){
            QModelIndex index = createIndex(0,0,parent);
            int row = parent->indexOf(children);
            beginRemoveRows(index, row, row);	
            parent->removeChild(children);
            endRemoveRows();
        }
    }
}
