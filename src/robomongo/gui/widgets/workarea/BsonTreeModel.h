#pragma once
#include <QAbstractItemModel>
#include "robomongo/core/Core.h"

namespace Robomongo
{
    class BsonTreeItem;

    class BsonTreeModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        typedef QAbstractItemModel BaseClass;

        explicit BsonTreeModel(const std::vector<MongoDocumentPtr> &documents,QObject *parent = 0);
        QVariant data(const QModelIndex &index, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);

        int rowCount(const QModelIndex &parent=QModelIndex()) const;
        int columnCount(const QModelIndex &parent) const;

        QVariant headerData(int section,Qt::Orientation orientation, int role=Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent= QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& index) const;

        void insertItem(BsonTreeItem *parent, BsonTreeItem *children);
        void removeitem(BsonTreeItem *children);
    public Q_SLOTS:
        
    private:
        BsonTreeItem *const _root;
    };
}
