#pragma once
#include <QTableView>

namespace Robomongo
{
    class BsonTableView : public QTableView
    {
        Q_OBJECT
    public:
        typedef QTableView BaseClass;
        explicit BsonTableView(QWidget *parent = 0);
    };
}
