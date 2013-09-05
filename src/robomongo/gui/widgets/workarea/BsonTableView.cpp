#include "robomongo/gui/widgets/workarea/BsonTableView.h"

#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

#include "robomongo/gui/widgets/workarea/BsonTableItem.h"
#include "robomongo/gui/utils/DialogUtils.h"
#include "robomongo/gui/dialogs/DocumentTextEditor.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{
    BsonTableView::BsonTableView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent) 
        :BaseClass(parent),
        _shell(shell),
        _queryInfo(queryInfo)
    {
#if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
        GuiRegistry::instance().setAlternatingColor(this);

        verticalHeader()->setDefaultAlignment(Qt::AlignLeft);
        horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
        //horizontalHeader()->setFixedHeight(25);   // commented because we shouldn't depend on heights in pixels - it may vary between platforms
        setStyleSheet("QTableView { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }");
        setShowGrid(false);
        setSelectionMode(QAbstractItemView::SingleSelection);

        setContextMenuPolicy(Qt::CustomContextMenu);
        VERIFY(connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&))));

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
    }

    QModelIndex BsonTableView::selectedIndex() const
    {
        QModelIndexList indexses = selectionModel()->selectedIndexes();
        int count = indexses.count();

        if (indexses.count() != 1)
            return QModelIndex();

        return indexses[0];
    }

    void BsonTableView::showContextMenu( const QPoint &point )
    {
        QModelIndex selectedInd = selectedIndex();
        if (selectedInd.isValid()){
            BsonTableItem *documentItem = QtUtils::item<BsonTableItem*>(selectedInd);

            bool isEditable = _queryInfo.isNull ? false : true;
            bool onItem = documentItem ? true : false;
            bool isSimple = false;
            if(documentItem){
                mongo::BSONElement el = documentItem->row(selectedInd.column()).second;
                isSimple = BsonUtils::isSimpleType(el) || BsonUtils::isUuidType(el);
            }

            QMenu menu(this);
            if (onItem && isEditable) menu.addAction(_editDocumentAction);
            if (onItem)               menu.addAction(_viewDocumentAction);
            if (isEditable)           menu.addAction(_insertDocumentAction);
            if (onItem && isSimple)   menu.addSeparator();
            if (onItem && isSimple)   menu.addAction(_copyValueAction);
            if (onItem && isEditable) menu.addSeparator();
            if (onItem && isEditable) menu.addAction(_deleteDocumentAction);

            QPoint menuPoint = mapToGlobal(point);
            menuPoint.setY(menuPoint.y() + horizontalHeader()->height());
            menuPoint.setX(menuPoint.x() + verticalHeader()->width());
            menu.exec(menuPoint);
        }
    }

    void BsonTableView::onDeleteDocument()
    {
        if (_queryInfo.isNull)
            return;

        QModelIndex selectedInd = selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTableItem *documentItem = QtUtils::item<BsonTableItem*>(selectedInd);
        if(!documentItem)
            return;

        mongo::BSONObj obj = documentItem->root();
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

    void BsonTableView::onEditDocument()
    {
        if (_queryInfo.isNull)
            return;

        QModelIndex selectedInd = selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTableItem *documentItem = QtUtils::item<BsonTableItem*>(selectedInd);
        if(!documentItem)
            return;

        mongo::BSONObj obj = documentItem->root();

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

    void BsonTableView::onViewDocument()
    {
        QModelIndex selectedInd = selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTableItem *documentItem = QtUtils::item<BsonTableItem*>(selectedInd);
        if(!documentItem)
            return;

        mongo::BSONObj obj = documentItem->root();

        std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, AppRegistry::instance().settingsManager()->uuidEncoding(), AppRegistry::instance().settingsManager()->timeZone());
        const QString &json = QtUtils::toQString(str);

        std::string server = _queryInfo.isNull ? "" : _queryInfo.serverAddress;
        std::string database = _queryInfo.isNull ? "" : _queryInfo.databaseName;
        std::string collection = _queryInfo.isNull ? "" : _queryInfo.collectionName;

        DocumentTextEditor *editor = new DocumentTextEditor(QtUtils::toQString(server),QtUtils::toQString(database), QtUtils::toQString(collection), json, true, this);

        editor->setWindowTitle("View Document");
        editor->show();
    }

    void BsonTableView::onInsertDocument()
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

    void BsonTableView::onCopyDocument()
    {
        QModelIndex selectedInd = selectedIndex();
        if (!selectedInd.isValid())
            return;

        BsonTableItem *documentItem = QtUtils::item<BsonTableItem*>(selectedInd);
        if(!documentItem)
            return;

        mongo::BSONElement element = documentItem->row(selectedInd.column()).second;

        if (!BsonUtils::isSimpleType(element) && BsonUtils::isUuidType(element))
            return;

        QClipboard *clipboard = QApplication::clipboard();
        std::string res;
        BsonUtils::buildJsonString(element,res,AppRegistry::instance().settingsManager()->uuidEncoding(),AppRegistry::instance().settingsManager()->timeZone());
        clipboard->setText(QtUtils::toQString(res));
    }

}
