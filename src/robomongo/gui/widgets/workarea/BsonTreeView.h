#pragma once

#include <QTreeView>

#include "robomongo/core/domain/Notifier.h"

namespace Robomongo
{
    class InsertDocumentResponse;

    class BsonTreeView : public QTreeView, public INotifierObserver
    {
        Q_OBJECT

    public:
        typedef QTreeView BaseClass;
        BsonTreeView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent = NULL);
        virtual QModelIndex selectedIndex() const;
        virtual QModelIndexList selectedIndexes() const;

    private Q_SLOTS:
        void onExpandRecursive();
        void showContextMenu(const QPoint &point);

    protected:
        virtual void resizeEvent(QResizeEvent *event);
        virtual void keyPressEvent(QKeyEvent *event);
        void expandNode(const QModelIndex &index);

    private:
        Notifier _notifier;
        QAction *_expandRecursive;
    };
}
