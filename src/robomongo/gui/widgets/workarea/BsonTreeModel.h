#pragma once
#include <vector>
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
        static const QIcon &getIcon(BsonTreeItem *item);
        explicit BsonTreeModel(const std::vector<MongoDocumentPtr> &documents, QObject *parent = 0);
        QVariant data(const QModelIndex &index, int role) const;

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent) const;

        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
        virtual QModelIndex parent(const QModelIndex& index) const;

        void insertItem(BsonTreeItem *parent, BsonTreeItem *children);
        void removeitem(BsonTreeItem *children);

        virtual void fetchMore(const QModelIndex &parent);
        virtual bool canFetchMore(const QModelIndex &parent) const;
        virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    protected:
        BsonTreeItem *const _root;
    };
}
