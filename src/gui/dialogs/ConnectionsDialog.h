#ifndef CONNECTIONSDIALOG_H
#define CONNECTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHash>
#include "Core.h"

namespace Robomongo
{
    class ConnectionListWidgetItem;
    class SettingsManager;
    class ConnectionRecord;

    /**
     * @brief Dialog allows select/edit/add/delete connections
     */
    class ConnectionsDialog : public QDialog
    {
        Q_OBJECT
    public:

        /**
         * @brief Creates dialog
         */
        ConnectionsDialog(SettingsManager *manager);

        /**
         * @brief ConnectionRecord, that was selected after pressing on
         * "Connect" button
         */
        ConnectionRecord *selectedConnection() const { return _selectedConnection; }

        /**
         * @brief This function is called when user clicks on "Connect" button.
         */
        virtual void accept();

    private slots:

        /**
         * @brief Add connection to the list widget
         */
        void add(ConnectionRecord *connection);

        /**
         * @brief Update specified connection (if it exists for this dialog)
         */
        void update(ConnectionRecord *connection);

        /**
         * @brief Remove specified connection (if it exists for this dialog)
         */
        void remove(ConnectionRecord *connection);

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

        /**
         * @brief ConnectionRecord, that was selected after pressing on
         * "Connect" button
         */
        ConnectionRecord *_selectedConnection;

        /**
         * @brief Main list widget
         */
        QListWidget *_listWidget;

        /**
         * @brief Settings manager
         */
        SettingsManager *_settingsManager;

        /**
         * @brief Hash that helps to connect ConnectionRecord with
         * ConnectionListWidgetItem*
         */
        QHash<ConnectionRecord *, ConnectionListWidgetItem *> _hash;
    };

    /**
     * @brief Simple ListWidgetItem that has several convenience methods.
     */
    class ConnectionListWidgetItem : public QListWidgetItem
    {
    public:

        /**
         * @brief Creates ConnectionListWidgetItem with specified ConnectionRecord
         */
        ConnectionListWidgetItem(ConnectionRecord *connection) { setConnection(connection); }

        /**
         * @brief Returns attached ConnectionRecord.
         */
        ConnectionRecord *connection() { return _connection; }

        /**
         * @brief Attach ConnectionRecord to this item
         */
        void setConnection(ConnectionRecord *connection);

    private:
        ConnectionRecord *_connection;
    };
}


#endif // CONNECTIONSDIALOG_H
