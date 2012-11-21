#ifndef EXPLORERSERVERTREEITEM_H
#define EXPLORERSERVERTREEITEM_H

#include <QTreeWidgetItem>
#include <QObject>
#include "Core.h"

namespace Robomongo
{
    class ExplorerServerTreeItem : public QObject, public QTreeWidgetItem
    {
        Q_OBJECT

    public:

        /*
        ** Constructs ExplorerServerTreeItem
        */
        ExplorerServerTreeItem(const MongoServerPtr &server);
        ~ExplorerServerTreeItem();

        /*
        ** Expand server tree item;
        */
        void expand();


    public slots:

        /*
        **
        */
        void databaseRefreshed(const QList<MongoDatabasePtr> &dbs);


    private:

        MongoServerPtr _server;

    };
}


#endif // EXPLORERSERVERTREEITEM_H
