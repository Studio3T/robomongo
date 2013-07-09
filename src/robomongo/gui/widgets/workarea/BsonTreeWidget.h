#pragma once

#include <QTreeWidget>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class BsonTreeItem;
    class InsertDocumentResponse;

    class BsonTreeWidget : public QTreeWidget
    {
        Q_OBJECT

    public:
        BsonTreeWidget(MongoShell *shell, QWidget *parent = NULL);
        ~BsonTreeWidget();

        void setDocuments(const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo = MongoQueryInfo());
        QIcon getIcon(MongoElementPtr element);

    protected:
        void contextMenuEvent(QContextMenuEvent *event);

    public slots:
        void ui_itemExpanded(QTreeWidgetItem *item);

    protected slots:
        void resizeEvent(QResizeEvent *event);
        void onDeleteDocument();
        void onEditDocument();
        void onViewDocument();
        void onInsertDocument();
        void onCopyDocument();
        void onExpandRecursive();
        void handle(InsertDocumentResponse *event);

    private:
        void expandNode(QTreeWidgetItem *item);
        /**
         * @returns selected BsonTreeItem, or NULL otherwise
         */
        BsonTreeItem *selectedBsonTreeItem();
        QString selectedJson();

        QAction *_deleteDocumentAction;
        QAction *_editDocumentAction;
        QAction *_viewDocumentAction;
        QAction *_insertDocumentAction;
        QAction *_copyValueAction;
        QAction *_expandRecursive;

        QList<MongoDocumentPtr> _documents;
        QMenu *_documentContextMenu;
        MongoQueryInfo _queryInfo;
        MongoShell *_shell;
        EventBus *_bus;
        SettingsManager *_settingsManager;
    };
}
