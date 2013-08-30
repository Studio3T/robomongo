#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

namespace Robomongo
{
    BsonTableModel::BsonTableModel(QObject *parent) 
        :BaseClass(parent)
    {
        _headerData.push_back("Key");
        _headerData.push_back("Value");
        _headerData.push_back("Type");
    }

    QVariant BsonTableModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();
        int col = index.column();
        BsonTreeItem *node = _items[index.row()];
        QVariant result;
        if(node){
            switch(col)
            {
            case 0:
                result = node->icon(0);
                break;
            case 1:
                result = node->text(1);
                break;
            case 2:
                result = node->text(2);
                break;
            default:
                break;
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
        return _items.size(); 
    }

    void BsonTableModel::setDocuments(const std::vector<MongoDocumentPtr> &documents)
    {
        _items.clear();
        BsonTreeItem *firstItem = NULL;
        size_t documentsCount = documents.size();
        for (int i = 0; i < documentsCount; ++i)
        {
            MongoDocumentPtr document = documents[i];
            BsonTreeItem *item = new BsonTreeItem(document, i);
            _items.push_back(item);
        }
    }

    int BsonTableModel::columnCount(const QModelIndex &parent)    const
    {
        return 3; 
    }

    QVariant BsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if(role != Qt::DisplayRole)
            return QVariant();
        if(orientation == Qt::Horizontal && role == Qt::DisplayRole){
            return _headerData[section]; 
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
