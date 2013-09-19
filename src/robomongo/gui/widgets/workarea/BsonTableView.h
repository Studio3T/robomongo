#pragma once
#include <QTableView>

#include "robomongo/core/domain/Notifier.h"

namespace Robomongo
{
    class BsonTableView : public QTableView , public INotifierObserver
    {
        Q_OBJECT
    public:
        typedef QTableView BaseClass;
        explicit BsonTableView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent = 0);     
        virtual QModelIndex selectedIndex() const;
        virtual QModelIndexList selectedIndexes() const;

    public Q_SLOTS:
        void showContextMenu(const QPoint &point);

    protected:
        virtual void keyPressEvent(QKeyEvent *event);

    private:
        Notifier _notifier;
    };
}
