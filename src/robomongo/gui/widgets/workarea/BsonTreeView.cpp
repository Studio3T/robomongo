#include "robomongo/gui/widgets/workarea/BsonTreeView.h"

#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QKeyEvent>

#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    BsonTreeView::BsonTreeView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent)
        : BaseClass(parent), _notifier(this, shell, queryInfo)
    {
#if defined(Q_OS_MAC)
        setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
        GuiRegistry::instance().setAlternatingColor(this);
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setContextMenuPolicy(Qt::CustomContextMenu);
        VERIFY(connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenu(const QPoint&))));

        _expandRecursive = new QAction("Expand Recursively", this);
        _expandRecursive->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Right));
        VERIFY(connect(_expandRecursive, SIGNAL(triggered()), SLOT(onExpandRecursive())));
        
        _collapseRecursive = new QAction(tr("Collapse Recursively"), this);
        _collapseRecursive->setShortcut(QKeySequence(Qt::ALT + Qt::Key_Left));
        VERIFY(connect(_collapseRecursive, SIGNAL(triggered()), SLOT(onCollapseRecursive())));

        setStyleSheet("QTreeView { border-left: 1px solid #c7c5c4; border-top: 1px solid #c7c5c4; }");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        header()->setSectionResizeMode(QHeaderView::Interactive);
#endif
    }

    void BsonTreeView::showContextMenu(const QPoint &point)
    {        
        QPoint menuPoint = mapToGlobal(point);
        menuPoint.setY(menuPoint.y() + header()->height());

        QModelIndexList indexes = selectedIndexes();
        if (detail::isMultiSelection(indexes)) {
            QMenu menu(this);
            
            menu.addAction(_expandRecursive);
            menu.addAction(_collapseRecursive);
            menu.addSeparator();
            
            _notifier.initMultiSelectionMenu(&menu);
            menu.exec(menuPoint);
        }
        else {

            QModelIndex selectedInd = selectedIndex();
            BsonTreeItem *documentItem = QtUtils::item<BsonTreeItem*>(selectedInd);

            QMenu menu(this);
            bool isSimple = false;
            if (documentItem) {
                isSimple = detail::isSimpleType(documentItem);
                if (detail::isDocumentType(documentItem)) {
                    menu.addAction(_expandRecursive);
                    menu.addAction(_collapseRecursive);
                    menu.addSeparator();
                }
            }

            _notifier.initMenu(&menu, documentItem);
            menu.exec(menuPoint);
        }
    }

    void BsonTreeView::resizeEvent(QResizeEvent *event)
    {
        BaseClass::resizeEvent(event);
        header()->resizeSections(QHeaderView::Stretch);
    }

    void BsonTreeView::keyPressEvent(QKeyEvent *event)
    {
        switch (event->key()) {
            case Qt::Key_Delete:
                _notifier.handleDeleteCommand();
                break;
            case Qt::Key_Backspace:
                // Cmd/Ctrl + Backspace
                if (event->modifiers() & Qt::ControlModifier)
                    _notifier.handleDeleteCommand();
                break;
            case Qt::Key_Right:
                if (event->modifiers() & Qt::AltModifier)
                    this->onExpandRecursive();
                break;
            case Qt::Key_Left:
                if (event->modifiers() & Qt::AltModifier)
                    this->onCollapseRecursive();
                break;
        }

        return BaseClass::keyPressEvent(event);
    }

    void BsonTreeView::expandNode(const QModelIndex &index)
    {
        if (index.isValid()) {
            BaseClass::expand(index);
            BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(index);
            for (unsigned i = 0; i < item->childrenCount(); ++i) {
                BsonTreeItem *tritem = item->child(i);
                if (tritem && detail::isDocumentType(tritem)) {
                    expandNode(model()->index(i, 0, index));
                }
            }
        }
    }
    
    void BsonTreeView::collapseNode(const QModelIndex &index)
    {
        if (index.isValid()) {
            BaseClass::collapse(index);
            BsonTreeItem *item = QtUtils::item<BsonTreeItem*>(index);
            for (unsigned i = 0; i < item->childrenCount(); ++i) {
                BsonTreeItem *tritem = item->child(i);
                if (tritem && detail::isDocumentType(tritem)) {
                    collapseNode(model()->index(i, 0, index));
                }
            }
        }
    }

    void BsonTreeView::onExpandRecursive()
    {
        QModelIndexList indexes = selectedIndexes();
        if (detail::isMultiSelection(indexes)) {
            for (int i = 0; i<indexes.count(); ++i)
                expandNode(indexes[i]);
        } else {
            expandNode(selectedIndex());
        }
    }

    void BsonTreeView::onCollapseRecursive()
    {
        QModelIndexList indexes = selectedIndexes();
        if (detail::isMultiSelection(indexes)) {
            for (int i = 0; i<indexes.count(); ++i)
                collapseNode(indexes[i]);
        } else {
            collapseNode(selectedIndex());
        }
    }

    /**
     * @returns selected BsonTreeItem, or NULL otherwise
     */
    QModelIndex BsonTreeView::selectedIndex() const
    {
        QModelIndexList indexes = detail::uniqueRows(selectionModel()->selectedRows());
        int count = indexes.count();

        if (indexes.count() != 1)
            return QModelIndex();

        return indexes[0];
    }

    QModelIndexList BsonTreeView::selectedIndexes() const
    {
        return detail::uniqueRows(selectionModel()->selectedRows(), true);
    }
}
