#pragma once

#include <QTreeView>

#include "robomongo/core/domain/Notifier.h"

namespace Robomongo
{
    class MongoServer;

    class BsonTreeView 
        : public QTreeView, public IViewObserver
    {
        Q_OBJECT

    public:
        typedef QTreeView BaseClass;
        BsonTreeView(QWidget *parent = NULL);
        virtual QModelIndex selectedIndex() const;
        virtual QModelIndexList selectedIndexes() const;
        void expandNode(const QModelIndex &index);
        
    private Q_SLOTS:
        void onExpandRecursive();
        void showContextMenu(const QPoint &point);

    protected:
        virtual void resizeEvent(QResizeEvent *event);
        virtual void keyPressEvent(QKeyEvent *event);
        
    private:
        QAction *_expandRecursive;    
    };
}
