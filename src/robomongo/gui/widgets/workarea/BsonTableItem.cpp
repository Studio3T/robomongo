#include "robomongo/gui/widgets/workarea/BsonTableItem.h"

using namespace mongo;

namespace Robomongo
{
    BsonTableItem::BsonTableItem(QObject *parent) 
        :BaseClass(parent)
    {
       
    }

    unsigned BsonTableItem::columnCount() const
    {
        return _columns.size();
    }

    QString BsonTableItem::column(int col) const
    {
        return _columns[col];
    }

    size_t BsonTableItem::findIndexColumn(const QString &col)
    {
        for(int i=0; i<_columns.size(); ++i)
        {
            if(_columns[i] == col){
                return i;
            }
        }
        return _columns.size();
    }

    size_t BsonTableItem::addColumn(const QString &col)
    {
        size_t column = findIndexColumn(col);
        if (column ==_columns.size()){
            _columns.push_back(col);
        }
        return column;
    }

    void BsonTableItem::addRow(size_t col,const QString &row)
    {
        if(_rows.size()<col+1){
            _rows.resize(col+1);
        }
        _rows[col]=row;
    }

    QString BsonTableItem::row(unsigned col) const
    {
        QString res;
        if(_rows.size()>col){
            res = _rows[col];
        }
        return res;
    }

    unsigned BsonTableItem::childrenCount() const
    {
        return _items.size();
    }

    void BsonTableItem::clear()
    {
        _items.clear();
    }

    void BsonTableItem::addChild(BsonTableItem *item)
    {
        _items.push_back(item);
    }

    BsonTableItem* BsonTableItem::child(unsigned pos)const
    {
        return _items[pos];
    }
}
