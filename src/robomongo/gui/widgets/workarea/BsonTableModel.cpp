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
        return 	count;
    }

    QModelIndex BsonTableModelProxy::parent( const QModelIndex& index ) const
    {
        return mapFromSource(sourceModel()->parent( mapToSource(index) ));
    }

    int BsonTableModelProxy::columnCount(const QModelIndex &parent) const
    {
        return _columns.size();
    }

    QModelIndex BsonTableModelProxy::mapFromSource( const QModelIndex & sourceIndex ) const
    {
        return index(sourceIndex.row(),sourceIndex.column(),sourceIndex.parent());
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
            hack->c = 0;
            hack->i = parent;
            hack->m = sourceModel();
        }
        return sourceIndex;
    }

    void BsonTableModelProxy::setSourceModel( QAbstractItemModel* model )
    {
        if (model) {
            BsonTreeItem *child = QtUtils::item<BsonTreeItem *>(model->index(0,0));
            if (child) {
                _root = qobject_cast<BsonTreeItem *>(child->parent());
                if (_root) {
                    int count = _root->childrenCount();
                    for (int i=0; i < count; ++i) {
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

        BsonTreeItem *child = QtUtils::item<BsonTreeItem *>(index);

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
            return BsonTreeModel::getIcon(child);
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

    QModelIndex BsonTableModelProxy::index( int row, int col, const QModelIndex& parent ) const
    {
        BsonTreeItem *node = QtUtils::item<BsonTreeItem *>(sourceModel()->index(row,0,parent));
        if (!node)
            return QModelIndex();

        BsonTreeItem *child = node->childByKey(_columns[col]);

        return createIndex( row, col, child );
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
