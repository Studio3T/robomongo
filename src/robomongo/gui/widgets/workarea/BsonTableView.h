#pragma once
#include <QTableView>

#include "robomongo/core/domain/MongoQueryInfo.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Robomongo
{
    class MongoShell;
    class BsonTableView : public QTableView
    {
        Q_OBJECT
    public:
        typedef QTableView BaseClass;
        explicit BsonTableView(MongoShell *shell, const MongoQueryInfo &queryInfo, QWidget *parent = 0);     

    public Q_SLOTS:
        void showContextMenu(const QPoint &point);

    private Q_SLOTS:
        void onDeleteDocument();
        void onEditDocument();
        void onViewDocument();
        void onInsertDocument();
        void onCopyDocument();

    private:
        QModelIndex selectedIndex() const;
        QAction *_deleteDocumentAction;
        QAction *_editDocumentAction;
        QAction *_viewDocumentAction;
        QAction *_insertDocumentAction;
        QAction *_copyValueAction;

        MongoShell *_shell;
        MongoQueryInfo _queryInfo;
    };
}
