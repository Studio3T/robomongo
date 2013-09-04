#pragma once
#include <QAbstractTableModel>
#include "robomongo/core/Core.h"

namespace Robomongo
{
    class BsonTableItem;

    class BsonTableModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        typedef QAbstractTableModel BaseClass;

        explicit BsonTableModel(const std::vector<MongoDocumentPtr> &documents,QObject *parent = 0);
        QVariant data(const QModelIndex &index, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);

        int rowCount(const QModelIndex &parent=QModelIndex()) const;
        int columnCount(const QModelIndex &parent) const;

        QVariant headerData(int section,Qt::Orientation orientation, int role=Qt::DisplayRole) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent= QModelIndex()) const;

    public Q_SLOTS:

    private:
        BsonTableItem *const _root;
    };
}
