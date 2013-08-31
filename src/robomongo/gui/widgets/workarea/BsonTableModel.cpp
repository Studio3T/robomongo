#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocumentIterator.h"
#include "robomongo/gui/widgets/workarea/BsonTableItem.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    BsonTableModel::BsonTableModel(const std::vector<MongoDocumentPtr> &documents, QObject *parent) 
        : BaseClass(parent) ,
        _root(new BsonTableItem(this))
    {
        for (std::vector<MongoDocumentPtr>::const_iterator it = documents.begin(); it!=documents.end(); ++it)
        {
            MongoDocumentPtr doc = (*it);
            MongoDocumentIterator iterator(doc.get());

            BsonTableItem *childItem = new BsonTableItem(parent);
            while(iterator.hasMore())
            {
                MongoElementPtr element = iterator.next();
                size_t col = _root->addColumn(QtUtils::toQString(element->fieldName()));
                childItem->addRow(col,element->stringValue());                
            }
            _root->addChild(childItem);
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

        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    }
}
