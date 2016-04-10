#pragma once
#include <vector>

#include <QAbstractProxyModel>

namespace Robomongo
{
    class BsonTreeItem;

    class BsonTableModelProxy : public QAbstractProxyModel
    {
        Q_OBJECT

    public:
        typedef QAbstractProxyModel BaseClass;
        typedef std::vector<QString> ColumnsValuesType;

        explicit BsonTableModelProxy(QObject *parent = 0);
        QVariant data(const QModelIndex &index, int role) const;

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent) const;

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        QModelIndex index( int row, int col, const QModelIndex& index ) const;
        virtual QModelIndex mapFromSource( const QModelIndex & sourceIndex ) const;
        virtual QModelIndex mapToSource( const QModelIndex &proxyIndex ) const;
        virtual void setSourceModel( QAbstractItemModel* model );
        virtual QModelIndex parent( const QModelIndex& index ) const;
        virtual QModelIndex sibling(int row, int column, const QModelIndex &idx) const;
    private:
        QString column(int col) const;
        size_t addColumn(const QString &col);
        size_t findIndexColumn(const QString &col) const;

        ColumnsValuesType _columns;
        BsonTreeItem *_root;
    };
}
