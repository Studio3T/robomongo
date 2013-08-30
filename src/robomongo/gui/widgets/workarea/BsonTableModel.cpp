#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

namespace Robomongo
{
    BsonTableModel::BsonTableModel(QObject *parent) 
        :BaseClass(parent)
    {
         _headerData.push_back("Дата");
         _headerData.push_back("Дата2");
         _headerData.push_back("Дата3");
    }

    QVariant BsonTableModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        return QVariant();
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
