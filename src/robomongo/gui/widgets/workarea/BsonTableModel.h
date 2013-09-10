#pragma once

#include "robomongo/gui/widgets/workarea/BsonTreeModel.h"
#include "robomongo/core/Core.h"

namespace Robomongo
{
    class BsonTableItem;

    class BsonTableModel : public BsonTreeModel
    {
        Q_OBJECT

    public:
        typedef BsonTreeModel BaseClass;
        typedef std::vector<QString> columnsValuesType;

        explicit BsonTableModel(const std::vector<MongoDocumentPtr> &documents,QObject *parent = 0);
        QVariant data(const QModelIndex &index, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);

        int rowCount(const QModelIndex &parent=QModelIndex()) const;
        int columnCount(const QModelIndex &parent) const;

        QVariant headerData(int section,Qt::Orientation orientation, int role=Qt::DisplayRole) const;

    private:
        QString column(int col) const;
        size_t addColumn(const QString &col);
        size_t findIndexColumn(const QString &col);

        columnsValuesType _columns;
    };
}
