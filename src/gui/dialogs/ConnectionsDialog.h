#ifndef CONNECTIONSDIALOG_H
#define CONNECTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHash>
#include "settings/ConnectionRecord.h"
#include "settings/SettingsManager.h"
#include <boost/scoped_ptr.hpp>
#include "Core.h"

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

    public:

        /**
         * @brief Creates dialog
         */
        ConnectionsDialog(SettingsManager *manager);

        /**
         * @brief ConnectionRecord, that was selected after pressing on "Connect" button
         */
        ConnectionRecordPtr selectedConnection() const { return _selectedConnection; }

        /**
         * @brief This function is called when user clicks on "Connect" button.
         */
        virtual void accept();

    private slots:

        /**
         * @brief Add connection to the list widget
         */
        void add(const ConnectionRecordPtr &connection);

        /**
         * @brief Update specified connection (if it exists for this dialog)
         */
        void update(const ConnectionRecordPtr &connection);

        /**
         * @brief Remove specified connection (if it exists for this dialog)
         */
        void remove(const ConnectionRecordPtr &connection);

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

        void layoutOfItemsChanged();

    private:

        /**
         * @brief ConnectionRecord, that was selected after pressing on "Connect" button
         */
        ConnectionRecordPtr _selectedConnection;

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
        QHash<ConnectionRecordPtr, ConnectionListWidgetItem *> _hash;

    };

    /**
     * @brief Simple ListWidgetItem that has several convenience methods.
     */
    class ConnectionListWidgetItem : public QListWidgetItem
    {
    public:

        ConnectionListWidgetItem(ConnectionRecordPtr connection)
        {
            setConnection(connection);
        }

        /**
         * @brief Returns attached ConnectionRecord.
         */
        ConnectionRecordPtr connection()
        {
            return _connection;
        }

        /**
         * @brief Attach ConnectionRecord to this item
         */
        void setConnection(ConnectionRecordPtr connection)
        {
            setText(connection->connectionName());
            _connection = connection;
        }

    private:
        ConnectionRecordPtr _connection;
    };
}


#endif // CONNECTIONSDIALOG_H
