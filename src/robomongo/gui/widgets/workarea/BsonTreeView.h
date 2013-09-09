#pragma once

#include <QTreeView>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class InsertDocumentResponse;
    class MongoShell;

    class BsonTreeView : public QTreeView
    {
        Q_OBJECT

    public:
        typedef QTreeView BaseClass;
        BsonTreeView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent = NULL);

    protected Q_SLOTS:
        void onDeleteDocument();
        void onEditDocument();
        void onViewDocument();
        void onInsertDocument();
        void onCopyDocument();

        void onExpandRecursive();
        void handle(InsertDocumentResponse *event);
    
    private Q_SLOTS:
        void showContextMenu(const QPoint &point);

    protected:
        virtual void resizeEvent(QResizeEvent *event);
        void expandNode(const QModelIndex &index);
        /**
         * @returns selected BsonTreeItem, or NULL otherwise
         */
        QModelIndex selectedIndex() const;

        QAction *_deleteDocumentAction;
        QAction *_editDocumentAction;
        QAction *_viewDocumentAction;
        QAction *_insertDocumentAction;
        QAction *_copyValueAction;

        QAction *_expandRecursive;

        MongoQueryInfo _queryInfo;
        MongoShell *_shell;
    };
}
