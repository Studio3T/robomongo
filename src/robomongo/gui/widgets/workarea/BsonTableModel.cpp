#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocumentIterator.h"
#include "robomongo/gui/widgets/workarea/BsonTableItem.h"
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    using namespace Robomongo;
    void parseDocument(BsonTableItem *root,const std::vector<MongoDocumentPtr> &documents)
    {
        for (std::vector<MongoDocumentPtr>::const_iterator it = documents.begin(); it!=documents.end(); ++it)
        {
            MongoDocumentPtr doc = (*it);
            MongoDocumentIterator iterator(doc.get());

            BsonTableItem *childItem = new BsonTableItem(root);
            while(iterator.hasMore())
            {
                MongoElementPtr element = iterator.next();
                size_t col = root->addColumn(QtUtils::toQString(element->fieldName()));
                if(element->isArray())
                {
                    int itemsCount = element->bsonElement().Array().size();
                    childItem->addRow(col,QString("Array[%1]").arg(itemsCount));
                }
                else if (element->isDocument()){
                    BsonTableItem *docChild = new BsonTableItem(childItem);
                    size_t colc = childItem->addColumn(QtUtils::toQString(element->fieldName()));
                    docChild->addRow(colc,element->stringValue());
                    childItem->addChild(docChild);
                    childItem->addRow(col,QString("{%1  Keys}").arg(childItem->childrenCount()));
                }
                else{
                    childItem->addRow(col,element->stringValue());
                }               
            }
            root->addChild(childItem);
        }
    }
}

namespace Robomongo
{
    BsonTableModel::BsonTableModel(const std::vector<MongoDocumentPtr> &documents, QObject *parent) 
        : BaseClass(parent) ,
        _root(new BsonTableItem(this))
    {
        parseDocument(_root,documents);
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
