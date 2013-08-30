#pragma once
#include <QTableView>

namespace Robomongo
{
    class MongoShell;
    class BsonTableView : public QTableView
    {
        Q_OBJECT
    public:
        typedef QTableView BaseClass;
        explicit BsonTableView(MongoShell *shell, QWidget *parent = 0);        
    };
}
