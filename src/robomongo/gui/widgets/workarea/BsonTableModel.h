#pragma once
#include <QAbstractTableModel>
#include "robomongo/core/Core.h"

namespace Robomongo
{
    class BsonTreeItem;

    class BsonTableModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        typedef QAbstractTableModel BaseClass;
        typedef std::vector<BsonTreeItem *> ItemsContainerType;
        typedef std::vector<QString> HeaderContainerType;

        explicit BsonTableModel(QObject *parent = 0);
        QVariant data(const QModelIndex &index, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);
        void setDocuments(const std::vector<MongoDocumentPtr> &documents);

        int rowCount(const QModelIndex &parent=QModelIndex()) const;
        int columnCount(const QModelIndex &parent) const;

        QVariant headerData(int section,Qt::Orientation orientation, int role=Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        public Q_SLOTS:

    private:
        HeaderContainerType _headerData;
        ItemsContainerType _items;
    };
}
