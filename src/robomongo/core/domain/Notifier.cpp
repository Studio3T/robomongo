#include "robomongo/core/domain/Notifier.h"
#include <QACtion>
#include <QMenu>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/BsonUtils.h" 
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

namespace Robomongo
{
    namespace detail
    {
        bool isSimpleType(Robomongo::BsonTreeItem *item)
        {
            return Robomongo::BsonUtils::isSimpleType(item->type())
                || Robomongo::BsonUtils::isUuidType(item->type(), item->binType());
        }

        bool isMultySelection(const QModelIndexList &indexes)
        {
            return indexes.count()>1;
        }

        bool isDocumentType(BsonTreeItem *item)
        {
            return Robomongo::BsonUtils::isDocument(item->type());
        }

        QModelIndexList uniqueRows(QModelIndexList indexses)
        {
            QModelIndexList result;
            for (QModelIndexList::const_iterator it = indexses.begin(); it!=indexses.end(); ++it)
            {
                QModelIndex isUnique = *it;
                Robomongo::BsonTreeItem *item = Robomongo::QtUtils::item<Robomongo::BsonTreeItem*>(isUnique);
                if(item){                
                    for (QModelIndexList::const_iterator jt = result.begin(); jt!=result.end(); ++jt)
                    {
                        Robomongo::BsonTreeItem *jItem = Robomongo::QtUtils::item<Robomongo::BsonTreeItem*>(*jt);
                        if (jItem && jItem->superParent() == item->superParent()){
                            isUnique = QModelIndex();
                            break;
                        }
                    }
                    if (isUnique.isValid()){
                        result.append(isUnique);
                    }
                }
            }        
            return result;
        }
    }
    INotifier::INotifier(IWatcher *watcher)
    {
        //VERIFY(connect(_shell->server(), SIGNAL(documentInserted()), this, SLOT(refresh())));
        QObject *qWatcher = dynamic_cast<QObject*>(watcher);
        VERIFY(qWatcher);

        QObject *qThis = dynamic_cast<QObject*>(this);
        //VERIFY(qThis);

        _deleteDocumentAction = new QAction("Delete Document...", qThis);
        VERIFY(QObject::connect(_deleteDocumentAction, SIGNAL(triggered()), qWatcher, SLOT(onDeleteDocument())));

        _deleteDocumentsAction = new QAction("Delete Documents...", qThis);
        VERIFY(QObject::connect(_deleteDocumentsAction, SIGNAL(triggered()), qWatcher, SLOT(onDeleteDocuments())));

        _editDocumentAction = new QAction("Edit Document...", qThis);
        VERIFY(QObject::connect(_editDocumentAction, SIGNAL(triggered()), qWatcher, SLOT(onEditDocument())));

        _viewDocumentAction = new QAction("View Document...", qThis);
        VERIFY(QObject::connect(_viewDocumentAction, SIGNAL(triggered()), qWatcher, SLOT(onViewDocument())));

        _insertDocumentAction = new QAction("Insert Document...", qThis);
        VERIFY(QObject::connect(_insertDocumentAction, SIGNAL(triggered()), qWatcher, SLOT(onInsertDocument())));

        _copyValueAction = new QAction("Copy Value", qThis);
        VERIFY(QObject::connect(_copyValueAction, SIGNAL(triggered()), qWatcher, SLOT(onCopyDocument())));

        _copyJsonAction = new QAction("Copy JSON", qThis);
        VERIFY(QObject::connect(_copyJsonAction, SIGNAL(triggered()), qWatcher, SLOT(onCopyJson()))); 
    }

    void INotifier::initMultiSelectionMenu(bool isEditable, QMenu *const menu)
    {
        if (isEditable) menu->addAction(_insertDocumentAction);
        if (isEditable) menu->addAction(_deleteDocumentsAction);
    }

    void INotifier::initMenu(bool isEditable, QMenu *const menu, BsonTreeItem *const item)
    {
        bool onItem = item ? true : false;

        bool isSimple = false;
        bool isDocument = false;
        if (item) {
            isSimple = detail::isSimpleType(item);
            isDocument = detail::isDocumentType(item);
        }         

        if (onItem && isEditable) menu->addAction(_editDocumentAction);
        if (onItem)               menu->addAction(_viewDocumentAction);
        if (isEditable)           menu->addAction(_insertDocumentAction);

        if (onItem && (isSimple || isDocument)) menu->addSeparator();

        if (onItem && isSimple)   menu->addAction(_copyValueAction);
        if (onItem && isDocument) menu->addAction(_copyJsonAction);
        if (onItem && isEditable) menu->addSeparator();
        if (onItem && isEditable) menu->addAction(_deleteDocumentAction);
    }
}
