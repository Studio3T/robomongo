#pragma once

#include <QTreeWidget>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class BsonTreeItem;
    class InsertDocumentResponse;
    class MongoShell;

    class BsonTreeWidget : public QTreeWidget
    {
        Q_OBJECT

    public:
        BsonTreeWidget(MongoShell *shell, QWidget *parent = NULL);
        void setDocuments(const std::vector<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo = MongoQueryInfo());    

    public Q_SLOTS:
        void ui_itemExpanded(QTreeWidgetItem *item);

    protected Q_SLOTS:
        void onDeleteDocument();
        void onEditDocument();
        void onViewDocument();
        void onInsertDocument();
        void onCopyDocument();
        void onExpandRecursive();
        void handle(InsertDocumentResponse *event);

    protected:
        virtual void resizeEvent(QResizeEvent *event);
        void contextMenuEvent(QContextMenuEvent *event);

    private:
        void expandNode(QTreeWidgetItem *item);
        /**
         * @returns selected BsonTreeItem, or NULL otherwise
         */
        BsonTreeItem *selectedBsonTreeItem() const;

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
