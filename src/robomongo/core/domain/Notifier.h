#pragma once
#include <QModelIndex>
QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class MongoShell;
    class BsonTreeItem;

    namespace detail
    {
        bool isSimpleType(BsonTreeItem *item);
    }

    class INotifierObserver
    {
    public:
        virtual QModelIndex selectedIndex() const = 0;
    protected:
        INotifierObserver(){};        
    };

    class Notifier : public QObject
    {
        Q_OBJECT

    public:
        typedef QObject BaseClass;
        Notifier(INotifierObserver *const observer, MongoShell *shell, const MongoQueryInfo &queryInfo,QObject *parent = NULL);
        void query();
        void initMenu(QMenu *const menu, BsonTreeItem *const item);

    public Q_SLOTS: 
        void onDeleteDocument();
        void onEditDocument();
        void onViewDocument();
        void onInsertDocument();
        void onCopyDocument();

    private:
        QAction *_deleteDocumentAction;
        QAction *_editDocumentAction;
        QAction *_viewDocumentAction;
        QAction *_insertDocumentAction;
        QAction *_copyValueAction;
        const MongoQueryInfo _queryInfo;

        MongoShell *_shell;
        INotifierObserver *const _observer;
    };
}
