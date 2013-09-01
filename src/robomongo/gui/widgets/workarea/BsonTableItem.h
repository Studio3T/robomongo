#pragma once

#include <vector>
#include <QObject>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    /**
     * @brief BSON tree item (represents array or object)
     */

    class BsonTableItem : public QObject
    {
        Q_OBJECT
    public:
        typedef QObject BaseClass;
        typedef std::vector<BsonTableItem*> childContainerType;
        typedef std::vector<QString> rowsValuesType;
        typedef std::vector<QString> columnsValuesType;

        explicit BsonTableItem(QObject *parent = 0);

        unsigned columnCount() const;
        QString column(int col) const;
        size_t addColumn(const QString &col);

        QString row(unsigned col) const;
        void addRow(size_t col,const QString &row);

        unsigned childrenCount() const;
        void clear();
        void addChild(BsonTableItem *item);
        BsonTableItem* child(unsigned pos)const;

        size_t findIndexColumn(const QString &col);
    private:
        columnsValuesType _columns;
        rowsValuesType _rows;
        childContainerType _items;
    };
}
