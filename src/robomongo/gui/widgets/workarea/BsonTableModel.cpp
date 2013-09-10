#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjiterator.h>
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{
    BsonTableModel::BsonTableModel(const std::vector<MongoDocumentPtr> &documents, QObject *parent) 
        : BaseClass(documents,parent)
    {
        int count = _root->childrenCount();
        for (int i=0;i<count;++i)
        {
            BsonTreeItem *child = _root->child(i);
            int countc = child->childrenCount();
            for (int j=0;j<countc;++j)
            {
                addColumn(child->child(j)->key());
            }
        }
    }

    QVariant BsonTableModel::data(const QModelIndex &index, int role) const
    {
        QVariant result;

        if (!index.isValid())
            return result;

        BsonTreeItem *node = dynamic_cast<BsonTreeItem *>(_root->child(index.row()));
        if (!node)
            return result;

        int col = index.column();

        BsonTreeItem *child = node->childByKey(_columns[col]);

        if (!child) {
            if (role == Qt::BackgroundRole) {
                return QBrush("#f5f3f2");
            }
            return result;
        }

        if (role == Qt::DisplayRole) {
            result = child->value();
        }
        else if (role == Qt::DecorationRole) {
            return getIcon(child);
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
        return _columns.size();
    }

    QVariant BsonTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if(role != Qt::DisplayRole)
            return QVariant();

        if(orientation == Qt::Horizontal && role == Qt::DisplayRole){
            return column(section); 
        }else{
            return QString("%1").arg( section + 1 ); 
        }
    }

    QString BsonTableModel::column(int col) const
    {
        return _columns[col];
    }

    size_t BsonTableModel::findIndexColumn(const QString &col)
    {
        for (int i = 0; i < _columns.size(); ++i) {
            if (_columns[i] == col) {
                return i;
            }
        }
        return _columns.size();
    }

    size_t BsonTableModel::addColumn(const QString &col)
    {
        size_t column = findIndexColumn(col);
        if (column == _columns.size()) {
            _columns.push_back(col);
        }
        return column;
    }
}
