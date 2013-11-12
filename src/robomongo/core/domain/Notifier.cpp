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

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/EventBus.h"

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

    Notifier::Notifier(INotifierObserver *const observer, MongoShell *shell, const MongoQueryInfo &queryInfo, QObject *parent) :
        BaseClass(parent),
        _observer(observer),
        _shell(shell),
        _queryInfo(queryInfo)
    {
        QWidget *wid = dynamic_cast<QWidget*>(_observer);
        VERIFY(connect(_shell->server(), SIGNAL(documentInserted()), this, SLOT(refresh())));

        _deleteDocumentAction = new QAction("Delete Document...", wid);
        VERIFY(connect(_deleteDocumentAction, SIGNAL(triggered()), this, SLOT(onDeleteDocument())));

        _deleteDocumentsAction = new QAction("Delete Documents...", wid);
        VERIFY(connect(_deleteDocumentsAction, SIGNAL(triggered()), this, SLOT(onDeleteDocuments())));

        _editDocumentAction = new QAction("Edit Document...", wid);
        VERIFY(connect(_editDocumentAction, SIGNAL(triggered()), this, SLOT(onEditDocument())));

        _viewDocumentAction = new QAction("View Document...", wid);
        VERIFY(connect(_viewDocumentAction, SIGNAL(triggered()), this, SLOT(onViewDocument())));

        _insertDocumentAction = new QAction("Insert Document...", wid);
        VERIFY(connect(_insertDocumentAction, SIGNAL(triggered()), this, SLOT(onInsertDocument())));

        _copyValueAction = new QAction("Copy Value", wid);
        VERIFY(connect(_copyValueAction, SIGNAL(triggered()), this, SLOT(onCopyDocument())));

        _copyJsonAction = new QAction("Copy JSON", wid);
        VERIFY(connect(_copyJsonAction, SIGNAL(triggered()), this, SLOT(onCopyJson())));        
    }

    void Notifier::initMenu(QMenu *const menu, BsonTreeItem *const item)
    {
        bool isEditable = _queryInfo._collectionInfo.isValid();
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

    void Notifier::initMultiSelectionMenu(QMenu *const menu)
    {
        bool isEditable = _queryInfo._collectionInfo.isValid();

        if (isEditable) menu->addAction(_insertDocumentAction);
        if (isEditable) menu->addAction(_deleteDocumentsAction);
    }

    void Notifier::deleteDocuments(std::vector<BsonTreeItem*> items, bool force)
    {
        bool isNeededRefresh = false;
        for (std::vector<BsonTreeItem*>::const_iterator it = items.begin(); it != items.end(); ++it) {
            BsonTreeItem * documentItem = *it;
            if (!documentItem)
                break;

            mongo::BSONObj obj = documentItem->superRoot();
            mongo::BSONElement id = obj.getField("_id");

            if (id.eoo()) {
                QMessageBox::warning(dynamic_cast<QWidget*>(_observer), "Cannot delete", "Selected document doesn't have _id field. \n"
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

            isNeededRefresh=true;
            _shell->server()->removeDocuments(query, _queryInfo._collectionInfo._ns);
        }

        if (isNeededRefresh)
            refresh();
    }

    void Notifier::refresh()
    {
        _shell->query(0, _queryInfo);
    }

    void Notifier::onDeleteDocuments()
    {
        if (!_queryInfo._collectionInfo.isValid())
            return;

        QModelIndexList selectedIndexes = _observer->selectedIndexes();
        if (!detail::isMultySelection(selectedIndexes))
            return;
        int answer = QMessageBox::question(dynamic_cast<QWidget*>(_observer), "Delete", QString("Do you want to delete %1 selected documents?").arg(selectedIndexes.count()));
        if (answer == QMessageBox::Yes) {
            std::vector<BsonTreeItem*> items;
            for (QModelIndexList::const_iterator it = selectedIndexes.begin(); it!= selectedIndexes.end(); ++it) {
                BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(*it);
                items.push_back(item);                
            }
            deleteDocuments(items,true);
        }
    }

    void Notifier::onDeleteDocument()
    {
        if (!_queryInfo._collectionInfo.isValid())
            return;

        QModelIndex selectedIndex = _observer->selectedIndex();
        if (!selectedIndex.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedIndex);
        std::vector<BsonTreeItem*> vec;
        vec.push_back(documentItem);
        return deleteDocuments(vec,false);
    }

    void Notifier::onEditDocument()
    {
        if (!_queryInfo._collectionInfo.isValid())
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

        DocumentTextEditor editor(_queryInfo._collectionInfo,
            json, false, dynamic_cast<QWidget*>(_observer));

        editor.setWindowTitle("Edit Document");
        int result = editor.exec();

        if (result == QDialog::Accepted) {
            _shell->server()->saveDocuments(editor.bsonObj(), _queryInfo._collectionInfo._ns);
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

        DocumentTextEditor *editor = new DocumentTextEditor(_queryInfo._collectionInfo,
            json, true, dynamic_cast<QWidget*>(_observer));

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void Notifier::onInsertDocument()
    {
        if (!_queryInfo._collectionInfo.isValid())
            return;

        DocumentTextEditor editor(_queryInfo._collectionInfo,
            "{\n    \n}", false, dynamic_cast<QWidget*>(_observer));

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle("Insert Document");

        int result = editor.exec();
        if (result != QDialog::Accepted)
            return;

        DocumentTextEditor::ReturnType obj = editor.bsonObj();
        for (DocumentTextEditor::ReturnType::const_iterator it = obj.begin(); it != obj.end(); ++it) {
            _shell->server()->insertDocument(*it, _queryInfo._collectionInfo._ns);
        }

        refresh();
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
