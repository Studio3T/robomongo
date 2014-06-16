#pragma once

#include <QModelIndex>

#include "robomongo/core/domain/MongoQueryInfo.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Robomongo
{
    class MongoShell;
    class BsonTreeItem;
    class InsertDocumentResponse;

    namespace detail
    {
        bool isSimpleType(BsonTreeItem *item);
        bool isObjectIdType(BsonTreeItem *item);
        bool isMultiSelection(const QModelIndexList &indexes);
        bool isDocumentType(BsonTreeItem *item);
        QModelIndexList uniqueRows(QModelIndexList indexes, bool returnSuperParents = false);
    }

    class INotifierObserver
    {
    public:
        virtual QModelIndex selectedIndex() const = 0;
        virtual QModelIndexList selectedIndexes() const = 0;

    protected:
        INotifierObserver() {}
    };

    class Notifier : public QObject
    {
        Q_OBJECT

    public:
        typedef QObject BaseClass;
        Notifier(INotifierObserver *const observer, MongoShell *shell, const MongoQueryInfo &queryInfo, QObject *parent = NULL);
        void initMenu(QMenu *const menu, BsonTreeItem *const item);
        void initMultiSelectionMenu(QMenu *const menu);

        void deleteDocuments(std::vector<BsonTreeItem*> items, bool force);

    public Q_SLOTS: 
        void onDeleteDocument();
        void onDeleteDocuments();
        void onEditDocument();
        void onViewDocument();
        void onInsertDocument();
        void onCopyDocument();
        void onCopyTimestamp();
        void onCopyJson();
        void handle(InsertDocumentResponse *event);

    private:
        QAction *_deleteDocumentAction;
        QAction *_deleteDocumentsAction;
        QAction *_editDocumentAction;
        QAction *_viewDocumentAction;
        QAction *_insertDocumentAction;
        QAction *_copyValueAction;
        QAction *_copyTimestampAction;
        QAction *_copyJsonAction;
        const MongoQueryInfo _queryInfo;

        MongoShell *_shell;
        INotifierObserver *const _observer;
    };
}
