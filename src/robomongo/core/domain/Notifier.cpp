#include "robomongo/core/domain/Notifier.h"

#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QMenu>

#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/events/MongoEvents.h"

#include "robomongo/shell/db/ptimeutil.h"

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{
    namespace detail
    {
        bool isSimpleType(Robomongo::BsonTreeItem const *item)
        {
            return BsonUtils::isSimpleType(item->type()) ||
                   BsonUtils::isUuidType(item->type(), item->binType());
        }

        bool isObjectIdType(Robomongo::BsonTreeItem *item)
        {
            return mongo::jstOID == item->type();
        }

        bool isMultiSelection(const QModelIndexList &indexes)
        {
            return indexes.count() > 1;
        }

        bool isDocumentType(BsonTreeItem const *item)
        {
            return BsonUtils::isDocument(item->type());
        }

        bool isArrayChild(BsonTreeItem const *item)
        {
            return BsonUtils::isArray(dynamic_cast<BsonTreeItem*>(item->parent())->type());
        }

        bool isDocumentRoot(BsonTreeItem const *item)
        {
            return ( item == item->superParent() );
        }

        /**
         * 
         * @param QModelIndexList indexes
         * @param bool returnSuperParents If TRUE, only indexes of super-parents will be in result list
         * @return QModelIndexList
         */
        QModelIndexList uniqueRows(QModelIndexList indexes, bool returnSuperParents)
        {
            QModelIndexList result;
            for (QModelIndexList::const_iterator it = indexes.begin(); it != indexes.end(); ++it)
            {
                QModelIndex isUnique = *it;
                Robomongo::BsonTreeItem *item = Robomongo::QtUtils::item<Robomongo::BsonTreeItem*>(isUnique);
                if (item) {
                    for (QModelIndexList::const_iterator jt = result.begin(); jt != result.end(); ++jt)
                    {
                        Robomongo::BsonTreeItem *jItem = Robomongo::QtUtils::item<Robomongo::BsonTreeItem*>(*jt);
                        if (jItem && jItem->superParent() == item->superParent()) {
                            isUnique = QModelIndex();
                            break;
                        }
                    }
                    if (isUnique.isValid()) {
                        if (returnSuperParents) {
                            // Move index onto "super parent" element before pushing it into result set
                            QModelIndex parent = isUnique.parent();
                            while (parent != QModelIndex()) {
                                isUnique = parent;
                                parent = parent.parent();
                            }
                        }
                        result.append(isUnique);
                    }
                }
            }        
            return result;
        }
    }

    Notifier::Notifier(INotifierObserver *const observer, MongoShell *shell, const MongoQueryInfo &queryInfo, QObject *parent) :
        BaseClass(parent),
        _observer(observer),
        _shell(shell),
        _queryInfo(queryInfo)
    {
        QWidget *wid = dynamic_cast<QWidget*>(_observer);
        AppRegistry::instance().bus()->subscribe(this, InsertDocumentResponse::Type, _shell->server());
        AppRegistry::instance().bus()->subscribe(this, RemoveDocumentResponse::Type, _shell->server());

        _deleteDocumentAction = new QAction("Delete Document...", wid);
        VERIFY(connect(_deleteDocumentAction, SIGNAL(triggered()), SLOT(onDeleteDocument())));

        _deleteDocumentsAction = new QAction("Delete Documents...", wid);
        VERIFY(connect(_deleteDocumentsAction, SIGNAL(triggered()), SLOT(onDeleteDocuments())));

        _editDocumentAction = new QAction("Edit Document...", wid);
        VERIFY(connect(_editDocumentAction, SIGNAL(triggered()), SLOT(onEditDocument())));

        _viewDocumentAction = new QAction("View Document...", wid);
        VERIFY(connect(_viewDocumentAction, SIGNAL(triggered()), SLOT(onViewDocument())));

        _insertDocumentAction = new QAction("Insert Document...", wid);
        VERIFY(connect(_insertDocumentAction, SIGNAL(triggered()), SLOT(onInsertDocument())));

        _copyValueAction = new QAction("Copy Value", wid);
        VERIFY(connect(_copyValueAction, SIGNAL(triggered()), SLOT(onCopyDocument())));

        _copyValueNameAction = new QAction("Copy Name", wid);
        VERIFY(connect(_copyValueNameAction, SIGNAL(triggered()), SLOT(onCopyNameDocument())));

        _copyValuePathAction = new QAction("Copy Path", wid);
        VERIFY(connect(_copyValuePathAction, SIGNAL(triggered()), SLOT(onCopyPathDocument())));

        _copyTimestampAction = new QAction("Copy Timestamp from ObjectId", wid);
        VERIFY(connect(_copyTimestampAction, SIGNAL(triggered()), SLOT(onCopyTimestamp())));

        _copyJsonAction = new QAction("Copy JSON", wid);
        VERIFY(connect(_copyJsonAction, SIGNAL(triggered()), SLOT(onCopyJson())));        
    }

    void Notifier::initMenu(QMenu *const menu, BsonTreeItem *const item)
    {
        bool isEditable = _queryInfo._info.isValid();
        bool onItem = item ? true : false;
        
        bool isSimple = false;
        bool isDocument = false;
        bool isObjectId = false;
        bool isNotArrayChild = false;
        bool isRoot = false;

        if (item) {
            isSimple = detail::isSimpleType(item);
            isDocument = detail::isDocumentType(item);
            isObjectId = detail::isObjectIdType(item);
            isNotArrayChild = !detail::isArrayChild(item);
            isRoot = detail::isDocumentRoot(item);
        }

        if (onItem && isEditable) menu->addAction(_editDocumentAction);
        if (onItem)               menu->addAction(_viewDocumentAction);
        if (isEditable)           menu->addAction(_insertDocumentAction);
        if (onItem && (isSimple || isDocument)) menu->addSeparator();
        if (onItem && isSimple)   menu->addAction(_copyValueAction);

        if (onItem && (isSimple || isDocument) && isNotArrayChild && !isRoot)
            menu->addAction(_copyValueNameAction);

        if (onItem && (isSimple || isDocument) && !isRoot)
            menu->addAction(_copyValuePathAction);

        if (onItem && isObjectId) menu->addAction(_copyTimestampAction);
        if (onItem && isDocument) menu->addAction(_copyJsonAction);
        if (onItem && isEditable) menu->addSeparator();
        if (onItem && isEditable) menu->addAction(_deleteDocumentAction);
    }

    void Notifier::initMultiSelectionMenu(QMenu *const menu)
    {
        bool isEditable = _queryInfo._info.isValid();

        if (isEditable) menu->addAction(_insertDocumentAction);
        if (isEditable) menu->addAction(_deleteDocumentsAction);
    }

    void Notifier::deleteDocuments(std::vector<BsonTreeItem*> const& items, bool force)
    {
        bool isNeededRefresh = false;

        int index = 0;
        for (auto const * const documentItem : items) {
            if (!documentItem)
                break;

            mongo::BSONObj obj = documentItem->superRoot();
            mongo::BSONElement id = obj.getField("_id");

            if (id.eoo()) {
                QMessageBox::warning(dynamic_cast<QWidget*>(_observer), "Cannot delete", 
                    "Selected document doesn't have _id field. \n"
                    "Maybe this is a system document that should be managed in a special way?");
                break;
            }

            mongo::BSONObjBuilder builder;
            builder.append(id);
            mongo::BSONObj bsonQuery = builder.obj();
            mongo::Query query(bsonQuery);

            if (!force) {
                // Ask user
                int answer = utils::questionDialog(dynamic_cast<QWidget*>(_observer), "Delete",
                    "Document", "%1 %2 with id:<br><b>%3</b>?", QtUtils::toQString(id.toString(false)));

                if (answer != QMessageBox::Yes)
                    break;
            }

            isNeededRefresh = true;

            RemoveDocumentCount removeCount = items.size() == 1 ? RemoveDocumentCount::ONE :  
                                                                  RemoveDocumentCount::MULTI;
            _shell->server()->removeDocuments(query, _queryInfo._info._ns, removeCount, index);
            ++index;
        }
    }

    void Notifier::handle(InsertDocumentResponse *event)
    {
        if (event->isError()) { // Error
            if (_shell->server()->connectionRecord()->isReplicaSet()) {
                // Insert document from tab results window (Notifier, OutputWindow widget)
                if (EventError::SetPrimaryUnreachable == event->error().errorCode()) {
                    AppRegistry::instance().bus()->publish(
                        new ReplicaSetRefreshed(_shell, event->error(), event->error().replicaSetInfo()));
                }
            }
            else  // single server
                QMessageBox::warning(NULL, "Database Error", QString::fromStdString(event->error().errorMessage()));
            
            return;
        }

        // Success
        _shell->query(0, _queryInfo);
    }

    void Notifier::handle(RemoveDocumentResponse *event)
    {
       if (event->isError()) {
            if (!(event->removeCount == RemoveDocumentCount::MULTI && event->index > 0))
                QMessageBox::warning(NULL, "Database Error", QString::fromStdString(event->error().errorMessage()));
       }
       else // Success
            _shell->query(0, _queryInfo);
    }

    void Notifier::onCopyNameDocument()
    {
        QModelIndex const& selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem const *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        if (!(detail::isSimpleType(documentItem) || detail::isDocumentType(documentItem) ||
            !detail::isArrayChild(documentItem) || !detail::isDocumentRoot(documentItem)))
            return;

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(QString::fromStdString(documentItem->fieldName()));
    }

    void Notifier::onCopyPathDocument()
    {
        QModelIndex const& selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem const *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        if (!(detail::isSimpleType(documentItem) || detail::isDocumentType(documentItem) ||
            !detail::isDocumentRoot(documentItem)))
            return;

        QStringList namesList;
        BsonTreeItem const *documentItemHelper = documentItem;

        while (!detail::isDocumentRoot(documentItemHelper)) {
            if (!detail::isArrayChild(documentItemHelper)) {
                namesList.push_front(QString::fromStdString(documentItemHelper->fieldName()));
            }

            documentItemHelper = dynamic_cast<BsonTreeItem*>(documentItemHelper->parent());
        }

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(namesList.join("."));
    }

    void Notifier::handleDeleteCommand()
    {
        if (_observer->selectedIndexes().count() > 1) 
            onDeleteDocuments();
        else 
            onDeleteDocument();        
    }

    void Notifier::onDeleteDocuments()
    {
        if (!_queryInfo._info.isValid())
            return;

        QModelIndexList selectedIndexes = _observer->selectedIndexes();
        if (!detail::isMultiSelection(selectedIndexes))
            return;

        int const answer = QMessageBox::question(dynamic_cast<QWidget*>(_observer), "Delete", 
                                           QString("Do you want to delete %1 selected documents?").
                                           arg(selectedIndexes.count()));

        if (QMessageBox::Yes == answer) {
            std::vector<BsonTreeItem*> items;
            for (auto index : selectedIndexes) 
                items.push_back(QtUtils::item<BsonTreeItem*>(index));
            
            deleteDocuments(items, true);
        }
    }

    void Notifier::onDeleteDocument()
    {
        if (!_queryInfo._info.isValid())
            return;

        QModelIndex selectedIndex = _observer->selectedIndex();
        if (!selectedIndex.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedIndex);
        std::vector<BsonTreeItem*> vec;
        vec.push_back(documentItem);
        return deleteDocuments(vec, false);
    }

    void Notifier::onEditDocument()
    {
        if (!_queryInfo._info.isValid())
            return;

        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->superRoot();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone());

        const QString &json = QtUtils::toQString(str);

        DocumentTextEditor editor(_queryInfo._info,
            json, false, dynamic_cast<QWidget*>(_observer));

        editor.setWindowTitle("Edit Document");
        int result = editor.exec();

        if (result == QDialog::Accepted) {
            _shell->server()->saveDocuments(editor.bsonObj(), _queryInfo._info._ns);
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

        mongo::BSONObj obj = documentItem->superRoot();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone());

        const QString &json = QtUtils::toQString(str);

        DocumentTextEditor *editor = new DocumentTextEditor(_queryInfo._info,
            json, true, dynamic_cast<QWidget*>(_observer));

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void Notifier::onInsertDocument()
    {
        if (!_queryInfo._info.isValid())
            return;

        DocumentTextEditor editor(_queryInfo._info,
            "{\n    \n}", false, dynamic_cast<QWidget*>(_observer));

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle("Insert Document");

        int result = editor.exec();
        if (result != QDialog::Accepted)
            return;

        DocumentTextEditor::ReturnType obj = editor.bsonObj();
        for (DocumentTextEditor::ReturnType::const_iterator it = obj.begin(); it != obj.end(); ++it) {
            _shell->server()->insertDocument(*it, _queryInfo._info._ns);
        }
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

    void Notifier::onCopyTimestamp()
    {
        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        if (!detail::isObjectIdType(documentItem))
            return;

        QClipboard *clipboard = QApplication::clipboard();

        // new Date(parseInt(this.valueOf().slice(0,8), 16)*1000);
        QString hexTimestamp = documentItem->value().mid(10, 8);
        bool ok;
        long long milliTimestamp = (long long)hexTimestamp.toLongLong(&ok, 16)*1000;

        bool isSupportedDate = (miutil::minDate < milliTimestamp) && (milliTimestamp < miutil::maxDate);

        boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        boost::posix_time::time_duration diff = boost::posix_time::millisec(milliTimestamp);
        boost::posix_time::ptime time = epoch + diff;

        if (isSupportedDate)
        {
            std::string date = miutil::isotimeString(time, false, false);
            clipboard->setText("ISODate(\""+QString::fromStdString(date)+"\")");
        }
        else {
            clipboard->setText("Error extracting ISODate()");
        }
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
         if (documentItem != documentItem->superParent()) {
             obj = obj[documentItem->fieldName()].Obj();
         }
         bool isArray = BsonUtils::isArray(documentItem->type());
         std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
                 AppRegistry::instance().settingsManager()->uuidEncoding(),
                 AppRegistry::instance().settingsManager()->timeZone(), isArray);

         const QString &json = QtUtils::toQString(str);
         clipboard->setText(json);
     }
}
