#ifndef CONNECTIONSDIALOG_H
#define CONNECTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHash>
#include "settings/ConnectionRecord.h"
#include "settings/SettingsManager.h"
#include <boost/scoped_ptr.hpp>

using namespace boost;

namespace Robomongo
{
    /**
     * @brief Forward declaration
     */
    class ConnectionListWidgetItem;

    /**
     * @brief Dialog allows select/edit/add/delete connections
     */
    class ConnectionsDialog : public QDialog
    {
        Q_OBJECT

    private:

        /**
         * @brief Main list widget
         */
        QListWidget *_listWidget;

        /**
         * @brief Settings manager
         */
        SettingsManager *_settingsManager;

        /**
         * @brief Hash that helps to connect ConnectionRecord with ConnectionListWidgetItem*
         */
        QHash<ConnectionRecord, ConnectionListWidgetItem *> _hash;

    public:

        /**
         * @brief Creates dialog
         */
        ConnectionsDialog(SettingsManager *manager);

        /**
         * @brief This function is called when user clicks on "Connect" button.
         */
        virtual void accept();

    private slots:

        /**
         * @brief Add connection to the list widget
         */
        void add(const ConnectionRecord &connection);

        /**
         * @brief Update specified connection (if it exists for this dialog)
         */
        void update(const ConnectionRecord &connection);

        /**
         * @brief Remove specified connection (if it exists for this dialog)
         */
        void remove(const ConnectionRecord &connection);

        /**
         * @brief Initiate 'add' action, usually when user clicked on Add button
         */
        void add();

        /**
         * @brief Initiate 'edit' action, usually when user clicked on Edit button
         */
        void edit();

        /**
         * @brief Initiate 'remove' action, usually when user clicked on Remove button
         */
        void remove();
    };

    /**
     * @brief Simple ListWidgetItem that has several convenience methods.
     */
    class ConnectionListWidgetItem : public QListWidgetItem
    {
    public:

        /**
         * @brief Returns attached ConnectionRecord.
         */
        ConnectionRecord connection()
        {
            QVariant var = data(Qt::UserRole);
            return var.value<ConnectionRecord>();
        }

        /**
         * @brief Attach ConnectionRecord to this item
         */
        void setConnection(ConnectionRecord connection)
        {
            setText(connection.connectionName());
            setData(Qt::UserRole, QVariant::fromValue<ConnectionRecord>(connection));
        }
    };
}


#endif // CONNECTIONSDIALOG_H
