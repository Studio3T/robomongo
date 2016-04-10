#include "robomongo/gui/widgets/workarea/BsonTableModel.h"

#include <QBrush>
#include <QIcon>

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/widgets/workarea/BsonTreeModel.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    BsonTableModelProxy::BsonTableModelProxy(QObject *parent) 
        : BaseClass(parent)
    {
       
    }

    int BsonTableModelProxy::rowCount(const QModelIndex &parent) const
    {
        int count = sourceModel()->rowCount(parent);
        return count;
    }

    QModelIndex BsonTableModelProxy::parent( const QModelIndex& index ) const
    {
        QModelIndex sourceParent = sourceModel()->parent( mapToSource(index) );
        return sourceParent;
    }

    int BsonTableModelProxy::columnCount(const QModelIndex &parent) const
    {
        return _columns.size();
    }

    QModelIndex BsonTableModelProxy::mapFromSource( const QModelIndex & sourceIndex ) const
    {
        int row = sourceIndex.row();
        int col = sourceIndex.column();

        BsonTreeItem *node = QtUtils::item<BsonTreeItem *>(sourceIndex);
        if (!node || _columns.size() <= col)
            return QModelIndex();

        BsonTreeItem *child = node->childByKey(_columns[col]);

        return createIndex( row, col, child );
    }

    QModelIndex BsonTableModelProxy::sibling(int row, int column, const QModelIndex &idx) const
    {
        return BaseClass::sibling(row, 0, idx);
    }

    QModelIndex BsonTableModelProxy::index( int row, int col, const QModelIndex& parent ) const
    {
        BsonTreeItem *node = QtUtils::item<BsonTreeItem *>(sourceModel()->index(row, 0, parent));
        if (!node || _columns.size() <= col)
            return QModelIndex();

        BsonTreeItem *child = node->childByKey(_columns[col]);

        return createIndex( row, col, child );
    }

    QModelIndex BsonTableModelProxy::mapToSource( const QModelIndex &proxyIndex ) const
    {
        if ( !proxyIndex.isValid() )
            return QModelIndex();

        Q_ASSERT( proxyIndex.model() == this );

        QModelIndex sourceIndex;
        BsonTreeItem *child = static_cast<BsonTreeItem *>(proxyIndex.internalPointer());
        if (child) {
            QtUtils::HackQModelIndex* hack = reinterpret_cast<QtUtils::HackQModelIndex*>(&sourceIndex);
            BsonTreeItem *parent = static_cast<BsonTreeItem *>(child->parent());
            hack->r = proxyIndex.row();
            hack->c = proxyIndex.column();
            hack->i = parent;
            hack->m = sourceModel();
        }
        return sourceIndex;
    }

    void BsonTableModelProxy::setSourceModel( QAbstractItemModel* model )
    {
        if (model) {
            BsonTreeItem *child = QtUtils::item<BsonTreeItem *>(model->index(0, 0));
            if (child) {
                _root = qobject_cast<BsonTreeItem *>(child->parent());
                if (_root) {
                    int count = _root->childrenCount();
                    for (int i = 0; i < count; ++i) {
                        BsonTreeItem *child = _root->child(i);
                        int countc = child->childrenCount();
                        for (int j = 0; j < countc; ++j) {
                            addColumn(child->child(j)->key());
                        }
                    }
                }
            }
        }
        return BaseClass::setSourceModel(model);
    }

    QVariant BsonTableModelProxy::data(const QModelIndex &index, int role) const
    {
        QVariant result;

        if (!index.isValid())
            return result;

        BsonTreeItem *node = QtUtils::item<BsonTreeItem *>(index);

        if (!node) {
            if (role == Qt::BackgroundRole) {
                return QBrush("#f5f3f2");
            }
            return result;
        }

        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            bool isCut = node->type() == mongo::String ||  node->type() == mongo::Code || node->type() == mongo::CodeWScope;  
            if (role == Qt::ToolTipRole) {
                result = isCut ? node->value() : node->value().left(500); 
            }
            else{
                result = isCut ? node->value() : node->value().simplified().left(300); 
            }
        }
        else if (role == Qt::DecorationRole) {
            return BsonTreeModel::getIcon(node);
        }

        return result;
    }

    QVariant BsonTableModelProxy::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role != Qt::DisplayRole)
            return QVariant();

        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            return column(section); 
        } else {
            return QString("%1").arg(section + 1);
        }
    }

    QString BsonTableModelProxy::column(int col) const
    {
        return _columns[col];
    }

    size_t BsonTableModelProxy::findIndexColumn(const QString &col) const
    {
        for (int i = 0; i < _columns.size(); ++i) {
            if (_columns[i] == col) {
                return i;
            }
        }
        return _columns.size();
    }

    size_t BsonTableModelProxy::addColumn(const QString &col)
    {
        size_t column = findIndexColumn(col);
        if (column == _columns.size()) {
            _columns.push_back(col);
        }
        return column;
    }
}
