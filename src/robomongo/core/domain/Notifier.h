#pragma once

#include <QModelIndex>

#include "robomongo/core/domain/MongoQueryInfo.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Robomongo
{
    class MongoServer;
    class BsonTreeItem;

    namespace detail
    {
        bool isSimpleType(BsonTreeItem *item);
        bool isMultySelection(const QModelIndexList &indexes);
        bool isDocumentType(BsonTreeItem *item);
        QModelIndexList uniqueRows(QModelIndexList indexses);
    }
    class IWatcher
    {
    public:
        virtual void onDeleteDocument() = 0;
        virtual void onDeleteDocuments() = 0;
        virtual void onEditDocument() = 0;
        virtual void onViewDocument() = 0;
        virtual void onInsertDocument() = 0;
        virtual void onCopyDocument() = 0;
        virtual void onCopyJson() = 0;
    protected:
        IWatcher(){};
    };

    class INotifier
    {
    public:
        virtual QModelIndex selectedIndex() const = 0;
        virtual QModelIndexList selectedIndexes() const = 0;

    protected:
        void initMenu(bool isEditable, QMenu *const menu, BsonTreeItem *const item);
        void initMultiSelectionMenu(bool isEditable, QMenu *const menu);
        INotifier(IWatcher *watcher);

        QAction *_deleteDocumentAction;
        QAction *_deleteDocumentsAction;
        QAction *_editDocumentAction;
        QAction *_viewDocumentAction;
        QAction *_insertDocumentAction;
        QAction *_copyValueAction;
        QAction *_copyJsonAction;
    };
}
