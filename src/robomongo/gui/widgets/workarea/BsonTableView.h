#pragma once
#include <QTableView>

#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class MongoShell;
    class BsonTableView : public QTableView
    {
        Q_OBJECT
    public:
        typedef QTableView BaseClass;
        explicit BsonTableView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent = 0);     

    public Q_SLOTS:
        void showContextMenu(const QPoint &a_point);
    private Q_SLOTS:

    private:
        MongoQueryInfo _queryInfo;
    };
}
