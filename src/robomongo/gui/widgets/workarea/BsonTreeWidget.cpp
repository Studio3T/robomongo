#include "robomongo/gui/widgets/workarea/BsonTreeWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QContextMenuEvent>

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/GuiRegistry.h"

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

using namespace mongo;
namespace Robomongo
{
    BsonTreeWidget::BsonTreeWidget(MongoShell *shell, QWidget *parent) 
        : QTreeWidget(parent), _shell(shell)
    {
#if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
        GuiRegistry::instance().setAlternatingColor(this);
        setHeaderLabels(QStringList() << "Key" << "Value" << "Type");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionResizeMode(0, QHeaderView::Stretch);
        header()->setSectionResizeMode(1, QHeaderView::Stretch);
        header()->setSectionResizeMode(2, QHeaderView::Stretch);
#endif
        setIndentation(15);
        setContextMenuPolicy(Qt::DefaultContextMenu);

        _deleteDocumentAction = new QAction("Delete Document", this);
        VERIFY(connect(_deleteDocumentAction, SIGNAL(triggered()), SLOT(onDeleteDocument())));

        _editDocumentAction = new QAction("Edit Document", this);
        VERIFY(connect(_editDocumentAction, SIGNAL(triggered()), SLOT(onEditDocument())));

        _viewDocumentAction = new QAction("View Document", this);
        VERIFY(connect(_viewDocumentAction, SIGNAL(triggered()), SLOT(onViewDocument())));

        _insertDocumentAction = new QAction("Insert Document", this);
        VERIFY(connect(_insertDocumentAction, SIGNAL(triggered()), SLOT(onInsertDocument())));

        _copyValueAction = new QAction("Copy Value", this);
        VERIFY(connect(_copyValueAction, SIGNAL(triggered()), SLOT(onCopyDocument())));

        _expandRecursive = new QAction("Expand Recursively", this);
         VERIFY(connect(_expandRecursive, SIGNAL(triggered()), SLOT(onExpandRecursive())));
         
        setStyleSheet("QTreeWidget { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionResizeMode(QHeaderView::Interactive);
        header()->setFixedHeight(25);
#endif
        VERIFY(connect(this, SIGNAL(itemExpanded(QTreeWidgetItem *)), SLOT(ui_itemExpanded(QTreeWidgetItem *))));
    }

    void BsonTreeWidget::setDocuments(const std::vector<MongoDocumentPtr> &documents,const MongoQueryInfo &queryInfo /* = MongoQueryInfo() */)
    {
        _queryInfo = queryInfo;

        setUpdatesEnabled(false);
        clear();

        BsonTreeItem *firstItem = NULL;

        QList<QTreeWidgetItem *> items;
        size_t documentsCount = documents.size();
        for (int i = 0; i < documentsCount; ++i)
        {
            MongoDocumentPtr document = documents[i];

            BsonTreeItem *item = new BsonTreeItem(document, i);
            items.append(item);

            if (i == 0)
                firstItem = item;
        }

        addTopLevelItems(items);
        setUpdatesEnabled(true);

        if (firstItem)
        {
            firstItem->expand();
            firstItem->setExpanded(true);
        }
    }

    void BsonTreeWidget::ui_itemExpanded(QTreeWidgetItem *treeItem)
    {
        BsonTreeItem *item = static_cast<BsonTreeItem *>(treeItem);
        item->expand();
    }

    void BsonTreeWidget::contextMenuEvent(QContextMenuEvent *event)
    {
        QTreeWidgetItem *item = itemAt(event->pos());
        BsonTreeItem *documentItem = dynamic_cast<BsonTreeItem *>(item);

        bool isEditable = _queryInfo.isNull ? false : true;
        bool onItem = documentItem ? true : false;
        bool isSimple = documentItem ? (documentItem->isSimpleType() || documentItem->isUuidType()) : false;

        QMenu menu(this);
        if (item && item->childIndicatorPolicy() == QTreeWidgetItem::ShowIndicator) {
            menu.addAction(_expandRecursive);
            menu.addSeparator();
        }
        if (onItem && isEditable) menu.addAction(_editDocumentAction);
        if (onItem)               menu.addAction(_viewDocumentAction);
        if (isEditable)           menu.addAction(_insertDocumentAction);
        if (onItem && isSimple)   menu.addSeparator();
        if (onItem && isSimple)   menu.addAction(_copyValueAction);
        if (onItem && isEditable) menu.addSeparator();
        if (onItem && isEditable) menu.addAction(_deleteDocumentAction);

        QPoint menuPoint = mapToGlobal(event->pos());
        menuPoint.setY(menuPoint.y() + header()->height());
        menu.exec(menuPoint);
    }

    void BsonTreeWidget::resizeEvent(QResizeEvent *event)
    {
        QTreeWidget::resizeEvent(event);
        header()->resizeSections(QHeaderView::Stretch);
    }

    void BsonTreeWidget::onDeleteDocument()
    {
        if (_queryInfo.isNull)
            return;

        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->rootDocument()->bsonObj();
        mongo::BSONElement id = obj.getField("_id");

        if (id.eoo()) {
            QMessageBox::warning(this, "Cannot delete", "Selected document doesn't have _id field. \n"
                                       "Maybe this is a system document that should be managed in a special way?");
            return;
        }

        mongo::BSONObjBuilder builder;
        builder.append(id);
        mongo::BSONObj bsonQuery = builder.obj();
        mongo::Query query(bsonQuery);

        // Ask user
        int answer = utils::questionDialog(this,"Delete","Document","%1 %2 with id:<br><b>%3</b>?",QtUtils::toQString(id.toString(false)));

        if (answer != QMessageBox::Yes)
            return ;

        _shell->server()->removeDocuments(query, _queryInfo.databaseName, _queryInfo.collectionName);
        _shell->query(0, _queryInfo);
    }

    void BsonTreeWidget::onEditDocument()
    {
        if (_queryInfo.isNull)
            return;

        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->rootDocument()->bsonObj();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone() );
        const QString &json = QtUtils::toQString(str);

        DocumentTextEditor editor(QtUtils::toQString(_queryInfo.serverAddress),
                                  QtUtils::toQString(_queryInfo.databaseName),
                                  QtUtils::toQString(_queryInfo.collectionName),
                                  json);

        editor.setWindowTitle("Edit Document");
        int result = editor.exec();
        activateWindow();

        if (result == QDialog::Accepted) {
            mongo::BSONObj obj = editor.bsonObj();
            AppRegistry::instance().bus()->subscribe(this, InsertDocumentResponse::Type);
            _shell->server()->saveDocument(obj, _queryInfo.databaseName, _queryInfo.collectionName);
        }
    }

    void BsonTreeWidget::onViewDocument()
    {
        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        mongo::BSONObj obj = documentItem->rootDocument()->bsonObj();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone());
        const QString &json = QtUtils::toQString(str);

        std::string server = _queryInfo.isNull ? "" : _queryInfo.serverAddress;
        std::string database = _queryInfo.isNull ? "" : _queryInfo.databaseName;
        std::string collection = _queryInfo.isNull ? "" : _queryInfo.collectionName;

        DocumentTextEditor *editor = new DocumentTextEditor(QtUtils::toQString(server),QtUtils::toQString(database), QtUtils::toQString(collection), json, true, this);

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void BsonTreeWidget::onInsertDocument()
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
        activateWindow();

        if (result == QDialog::Accepted) {
            mongo::BSONObj obj = editor.bsonObj();
            _shell->server()->insertDocument(obj, _queryInfo.databaseName, _queryInfo.collectionName);
            _shell->query(0, _queryInfo);
        }
    }

    void BsonTreeWidget::onCopyDocument()
    {
        BsonTreeItem *documentItem = selectedBsonTreeItem();
        if (!documentItem)
            return;

        MongoElementPtr element = documentItem->element();

        if (!element->isSimpleType() && !element->isUuidType())
            return;

        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(element->stringValue());
    }

    void BsonTreeWidget::expandNode(QTreeWidgetItem *item)
    {
        if(item){
            ui_itemExpanded(item);
            item->setExpanded(true);
            for(int i = 0;i<item->childCount();++i){
                QTreeWidgetItem *tritem = item->child(i);
                if(tritem&&tritem->childIndicatorPolicy()==QTreeWidgetItem::ShowIndicator){
                    expandNode(tritem);
                }
            }
        }
    }

    void BsonTreeWidget::onExpandRecursive()
    {
        expandNode(currentItem());
    }

    void BsonTreeWidget::handle(InsertDocumentResponse *event)
    {
        AppRegistry::instance().bus()->unsubscibe(this);
        _shell->query(0, _queryInfo);
    }

    /**
     * @returns selected BsonTreeItem, or NULL otherwise
     */
    BsonTreeItem *BsonTreeWidget::selectedBsonTreeItem() const
    {
        QList<QTreeWidgetItem*> items = selectedItems();

        if (items.count() != 1)
            return NULL;

        QTreeWidgetItem *item = items[0];

        if (!item)
            return NULL;

        return dynamic_cast<BsonTreeItem *>(item);
    }
}
