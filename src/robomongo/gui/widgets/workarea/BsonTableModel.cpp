#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include "robomongo/gui/widgets/workarea/BsonTableItem.h"

namespace Robomongo
{
    BsonTableModel::BsonTableModel(const std::vector<MongoDocumentPtr> &documents,QObject *parent) 
        :BaseClass(parent)
        ,_root(new BsonTableItem(NULL))
    {
        _headerData.push_back("Key");
        _headerData.push_back("Value");
        _headerData.push_back("Type");
        
        size_t documentsCount = documents.size();
        for (int i = 0; i < documentsCount; ++i)
        {
            MongoDocumentPtr document = documents[i];
            BsonTableItem *item = new BsonTableItem(document);
            _root->addChild(item);
        }
    }

    QVariant BsonTableModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();
        int col = index.column();
        BsonTableItem *node = _root->child(index.row());
        QVariant result;
        if(node){
            switch(col)
            {
            case 0:
             //   result = node->text();
                break;
            case 1:
             //   result = node->typeText();
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
        BsonTableItem *item = static_cast<BsonTableItem*>(parent.internalPointer());
        if(item){
            return item->childrenCount(); 
        }
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
