#pragma once

#include <QDialog>
#include <QTreeWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

#include "robomongo/core/Core.h"
#include "robomongo/core/settings/ConnectionSettings.h"

namespace Robomongo
{
    class ReplicasetConnectionSettings;

    /**
     * @brief Dialog allows select/edit/add/delete connections
     */
    class ReplicasetDialog : public QDialog
    {
        Q_OBJECT

    public:
        typedef std::vector<QTreeWidgetItem*> ConnectionListItemContainerType; 
        /**
         * @brief Creates dialog
         */
        ReplicasetDialog(ReplicasetConnectionSettings *connection, QWidget *parent = 0);
        virtual void accept();

    private Q_SLOTS:
        void linkActivated(const QString &link);

        /**
         * @brief Add connection to the list widget
         */
        void add(ConnectionSettings *connection);

        /**
         * @brief Initiate 'add' action, usually when user clicked on Add button
         */
        void add();

        /**
         * @brief Initiate 'edit' action, usually when user clicked on Edit
         * button
         */
        void edit();

        /**
         * @brief Initiate 'remove' action, usually when user clicked on Remove
         * button
         */
        void remove();

        /**
         * @brief Initiate 'clone' action, usually when user clicked on Clone
         * button
         */
        void clone();

        /**
         * @brief Handles ListWidget layoutChanged() signal
         */
        void listWidget_layoutChanged();
    private:
        QLineEdit *_nameConnection;
        QLineEdit *_replicasetName;
        QTreeWidget *_listWidget;
        ConnectionListItemContainerType _connectionItems;
        ReplicasetConnectionSettings *_repConnection;
    };    

    class ReplicaSetConnectionsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        ReplicaSetConnectionsTreeWidget();

    Q_SIGNALS:
        void layoutChanged();

    protected:
        void dropEvent(QDropEvent *event);
    };
}
