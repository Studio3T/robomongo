#pragma once

#include <QModelIndex>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

namespace Robomongo
{
    class BsonTreeItem;

    namespace detail
    {
        bool isSimpleType(BsonTreeItem *item);
        bool isMultySelection(const QModelIndexList &indexes);
        bool isDocumentType(BsonTreeItem *item);
        QModelIndexList uniqueRows(QModelIndexList indexses);
    }

    class IViewObserver;
    class Notifier :public QObject
    {
        Q_OBJECT
    public:
        Notifier(IViewObserver *observer);
        void initMenu(bool isEditable, QMenu *const menu, BsonTreeItem *const item) const;
        void initMultiSelectionMenu(bool isEditable, QMenu *const menu) const;
        void inProcessDeleteDocument(bool force);

    Q_SIGNALS:
        void deletedDocument(BsonTreeItem* item, bool force);
        void editedDocument(BsonTreeItem *item);
        void viewDocument(BsonTreeItem *item);
        void createdDocument();

    private Q_SLOTS:
        void onDeleteDocument();
        void onDeleteDocuments();
        void onEditDocument();
        void onViewDocument();
        void onInsertDocument();
        void onCopyDocument();
        void onCopyJson();

    protected:
        void deleteDocumentsImpl(std::vector<BsonTreeItem*> items, bool force);

        IViewObserver *_observer;
        QAction *_deleteDocumentAction;
        QAction *_deleteDocumentsAction;
        QAction *_editDocumentAction;
        QAction *_viewDocumentAction;
        QAction *_insertDocumentAction;
        QAction *_copyValueAction;
        QAction *_copyJsonAction;
    };

    class IViewObserver
    {
    public:
        virtual QModelIndex selectedIndex() const = 0;
        virtual QModelIndexList selectedIndexes() const = 0;
        const Notifier &notifier() const
        {
            return _notifier;
        }
    protected:
        IViewObserver();
        Notifier _notifier;  
    };
}
