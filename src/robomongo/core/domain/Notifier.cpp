#include "robomongo/core/domain/Notifier.h"
#include <QAction>
#include <QMenu>
#include <QClipboard>
#include <QDialog>
#include <QMessageBox>
#include <QApplication>
#include <mongo/client/dbclientinterface.h>
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/BsonUtils.h" 
#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/core/utils/BsonUtils.h" 
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"

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
    
    IViewObserver::IViewObserver()
        :_notifier(this)
    {

    }

    Notifier::Notifier(IViewObserver *observer)
        :_observer(observer)
    {
        _deleteDocumentAction = new QAction("Delete Document...", this);
        VERIFY(QObject::connect(_deleteDocumentAction, SIGNAL(triggered()), this, SLOT(onDeleteDocument())));

        _deleteDocumentsAction = new QAction("Delete Documents...", this);
        VERIFY(QObject::connect(_deleteDocumentsAction, SIGNAL(triggered()), this, SLOT(onDeleteDocuments())));

        _editDocumentAction = new QAction("Edit Document...", this);
        VERIFY(QObject::connect(_editDocumentAction, SIGNAL(triggered()), this, SLOT(onEditDocument())));

        _viewDocumentAction = new QAction("View Document...", this);
        VERIFY(QObject::connect(_viewDocumentAction, SIGNAL(triggered()), this, SLOT(onViewDocument())));

        _insertDocumentAction = new QAction("Insert Document...", this);
        VERIFY(QObject::connect(_insertDocumentAction, SIGNAL(triggered()), this, SLOT(onInsertDocument())));

        _copyValueAction = new QAction("Copy Value", this);
        VERIFY(QObject::connect(_copyValueAction, SIGNAL(triggered()), this, SLOT(onCopyDocument())));

        _copyJsonAction = new QAction("Copy JSON", this);
        VERIFY(QObject::connect(_copyJsonAction, SIGNAL(triggered()), this, SLOT(onCopyJson()))); 
    }

    void Notifier::initMultiSelectionMenu(bool isEditable, QMenu *const menu) const
    {
        if (isEditable) menu->addAction(_insertDocumentAction);
        if (isEditable) menu->addAction(_deleteDocumentsAction);
    }

    void Notifier::initMenu(bool isEditable, QMenu *const menu, BsonTreeItem *const item) const
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

    void Notifier::inProcessDeleteDocument(bool force)
    {
        QModelIndexList selectedIndexes = _observer->selectedIndexes();
        if (detail::isMultySelection(selectedIndexes)){
            onDeleteDocuments();
        }
        else {
            onDeleteDocument();
        }
    }

    void Notifier::onDeleteDocuments()
    {
        QModelIndexList selectedIndexes = _observer->selectedIndexes();
        if (!detail::isMultySelection(selectedIndexes))
            return;

        std::vector<BsonTreeItem*> items;
        for (QModelIndexList::const_iterator it = selectedIndexes.begin(); it!= selectedIndexes.end(); ++it) {
            BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(*it);
            items.push_back(item);                
        }

        deleteDocumentsImpl(items, true);
    }

    void Notifier::onEditDocument()
    {
        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        emit editedDocument(documentItem);       
    }

    void Notifier::onDeleteDocument()
    {
        QModelIndex selectedIndex = _observer->selectedIndex();
        if (!selectedIndex.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedIndex);
        if (!documentItem)
            return;

        emit deletedDocument(documentItem, false);
    }

    void Notifier::deleteDocumentsImpl(std::vector<BsonTreeItem *> items, bool force)
    {
        QWidget *wid = dynamic_cast<QWidget*>(_observer);
        VERIFY(wid);

        int answer = QMessageBox::question(wid, "Delete", QString("Do you want to delete %1 selected documents?").arg(items.size()));
        if (answer == QMessageBox::Yes) {
            for (std::vector<BsonTreeItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
                
                BsonTreeItem * documentItem = *it;
                if (!documentItem)
                    break;

                emit deletedDocument(documentItem, force);
            }
        }
    }

    void Notifier::onViewDocument()
    {
        QModelIndex selectedIndex = _observer->selectedIndex();
        if (!selectedIndex.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedIndex);
        if (!documentItem)
            return;

        emit viewDocument(documentItem);
    }

    void Notifier::onInsertDocument()
    {
        emit createdDocument();
    }

    void Notifier::onCopyDocument()
    {
        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        if (!detail::isSimpleType(documentItem))
            return;

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(documentItem->value());
    }

    void Notifier::onCopyJson()
    {
        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        if (!detail::isDocumentType(documentItem))
            return;

        QClipboard *clipboard = QApplication::clipboard();
        mongo::BSONObj obj = documentItem->root();
        if (documentItem != documentItem->superParent()){
            obj = obj[QtUtils::toStdString(documentItem->key())].Obj();
        }
        bool isArray = BsonUtils::isArray(documentItem->type());
        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone(),isArray);

        const QString &json = QtUtils::toQString(str);
        clipboard->setText(json);
    }
}
