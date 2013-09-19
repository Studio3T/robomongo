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
            return indexes.count()!=0;
        }
    }

    Notifier::Notifier(INotifierObserver *const observer, MongoShell *shell, const MongoQueryInfo &queryInfo, QObject *parent) :
        BaseClass(parent),
        _observer(observer),
        _shell(shell),
        _queryInfo(queryInfo)
    {
        QWidget *wid = dynamic_cast<QWidget*>(_observer);

        _deleteDocumentAction = new QAction("Delete Document", wid);
        VERIFY(connect(_deleteDocumentAction, SIGNAL(triggered()), SLOT(onDeleteDocument())));

        _deleteDocumentsAction = new QAction("Delete Documents", wid);
        VERIFY(connect(_deleteDocumentsAction, SIGNAL(triggered()), SLOT(onDeleteDocuments())));

        _editDocumentAction = new QAction("Edit Document", wid);
        VERIFY(connect(_editDocumentAction, SIGNAL(triggered()), SLOT(onEditDocument())));

        _viewDocumentAction = new QAction("View Document", wid);
        VERIFY(connect(_viewDocumentAction, SIGNAL(triggered()), SLOT(onViewDocument())));

        _insertDocumentAction = new QAction("Insert Document", wid);
        VERIFY(connect(_insertDocumentAction, SIGNAL(triggered()), SLOT(onInsertDocument())));

        _copyValueAction = new QAction("Copy Value", wid);
        VERIFY(connect(_copyValueAction, SIGNAL(triggered()), SLOT(onCopyDocument())));
    }

    void Notifier::initMenu(QMenu *const menu, BsonTreeItem *const item)
    {
        bool isEditable = _queryInfo.isNull ? false : true;
        bool onItem = item ? true : false;
        
        bool isSimple = false;
        if (item) {
            isSimple = detail::isSimpleType(item);
        }

        if (onItem && isEditable) menu->addAction(_editDocumentAction);
        if (onItem)               menu->addAction(_viewDocumentAction);
        if (isEditable)           menu->addAction(_insertDocumentAction);
        if (onItem && isSimple)   menu->addSeparator();
        if (onItem && isSimple)   menu->addAction(_copyValueAction);
        if (onItem && isEditable) menu->addSeparator();
        if (onItem && isEditable) menu->addAction(_deleteDocumentAction);
    }

    void Notifier::initMultiSelectionMenu(QMenu *const menu)
    {
        bool isEditable = _queryInfo.isNull ? false : true;

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

            mongo::BSONObj obj = documentItem->root();
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
            _shell->server()->removeDocuments(query, _queryInfo.databaseName, _queryInfo.collectionName);
        }

        if (isNeededRefresh)
            _shell->query(0, _queryInfo);
    }

    void Notifier::onDeleteDocuments()
    {
        if (_queryInfo.isNull)
            return;

        QModelIndexList selectedIndexes = _observer->selectedIndexes();
        if (!detail::isMultySelection(selectedIndexes))
            return;
        int answer = QMessageBox::question(dynamic_cast<QWidget*>(_observer), "Delete", "Do you want to delete all selected documents?");
        if (answer == QMessageBox::Yes){
            std::vector<BsonTreeItem*> items;
            for (QModelIndexList::const_iterator it = selectedIndexes.begin(); it!= selectedIndexes.end(); ++it)
            {
                BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(*it);
                items.push_back(item);                
            }
            deleteDocuments(items,true);
        }
    }

    void Notifier::onDeleteDocument()
    {
        if (_queryInfo.isNull)
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
        if (_queryInfo.isNull)
            return;

        QModelIndex selectedInd = _observer->selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->root();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone());

        const QString &json = QtUtils::toQString(str);

        DocumentTextEditor editor(QtUtils::toQString(_queryInfo.serverAddress),
            QtUtils::toQString(_queryInfo.databaseName),
            QtUtils::toQString(_queryInfo.collectionName),
            json);

        editor.setWindowTitle("Edit Document");
        int result = editor.exec();

        if (result == QDialog::Accepted) {
            _shell->server()->saveDocuments(editor.bsonObj(), _queryInfo.databaseName, _queryInfo.collectionName);
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

        mongo::BSONObj obj = documentItem->root();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1,
            AppRegistry::instance().settingsManager()->uuidEncoding(),
            AppRegistry::instance().settingsManager()->timeZone());

        const QString &json = QtUtils::toQString(str);

        std::string server = _queryInfo.isNull ? "" : _queryInfo.serverAddress;
        std::string database = _queryInfo.isNull ? "" : _queryInfo.databaseName;
        std::string collection = _queryInfo.isNull ? "" : _queryInfo.collectionName;

        DocumentTextEditor *editor = new DocumentTextEditor(
            QtUtils::toQString(server), QtUtils::toQString(database), QtUtils::toQString(collection),
            json, true, dynamic_cast<QWidget*>(_observer));

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void Notifier::onInsertDocument()
    {
        if (_queryInfo.isNull)
            return;

        DocumentTextEditor editor(QtUtils::toQString(_queryInfo.serverAddress),
            QtUtils::toQString(_queryInfo.databaseName),
            QtUtils::toQString(_queryInfo.collectionName),
            "{\n    \n}");

        editor.setCursorPosition(1, 4);
        editor.setWindowTitle("Insert Document");

        int result = editor.exec();
        if (result != QDialog::Accepted)
            return;

        DocumentTextEditor::ReturnType obj = editor.bsonObj();
        for (DocumentTextEditor::ReturnType::const_iterator it = obj.begin(); it != obj.end(); ++it) {
            _shell->server()->insertDocument(*it, _queryInfo.databaseName, _queryInfo.collectionName);
        }

        _shell->query(0, _queryInfo);
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
}
