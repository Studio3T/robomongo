#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjiterator.h>
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/gui/widgets/workarea/BsonTableItem.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    using namespace Robomongo;
    void parseDocument(BsonTableItem *root,const mongo::BSONObj &doc)
    {            
            mongo::BSONObjIterator iterator(doc);
            BsonTableItem *childItem = new BsonTableItem(root);
            while(iterator.more())
            {
                mongo::BSONElement element = iterator.next();
                size_t col = root->addColumn(QtUtils::toQString(std::string(element.fieldName())));
                if(BsonUtils::isArray(element))
                {
                    int itemsCount = element.Array().size();
                    childItem->addRow(col,QString("Array[%1]").arg(itemsCount));
                }
                else if (BsonUtils::isDocument(element)){
                    parseDocument(childItem,element.Obj());
                    childItem->addRow(col,QString("{%1  Keys}").arg(childItem->columnCount()));
                }
                else{
                    std::string result;
                    BsonUtils::buildJsonString(element,result,AppRegistry::instance().settingsManager()->uuidEncoding(),AppRegistry::instance().settingsManager()->timeZone());
                    childItem->addRow(col,QtUtils::toQString(result));
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
        if (!index.isValid())
            return QVariant();

        int col = index.column();
        QVariant result;

        if (role == Qt::DisplayRole) {
            BsonTableItem *node = _root->child(index.row());
            if(node){
                result = node->row(col);
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
}
