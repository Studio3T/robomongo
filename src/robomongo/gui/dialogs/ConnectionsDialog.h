#pragma once

#include <QDialog>
#include <QTreeWidget>

#include "robomongo/core/Core.h"

namespace Robomongo
{
    class ConnectionListWidgetItem;
    class SettingsManager;
    class ConnectionSettings;

    /**
     * @brief Dialog allows select/edit/add/delete connections
     */
    class ConnectionsDialog : public QDialog
    {
        Q_OBJECT

    public:
        typedef std::vector<ConnectionListWidgetItem *> ConnectionListItemContainerType; 
        /**
         * @brief Creates dialog
         */
        ConnectionsDialog(SettingsManager *manager, QWidget *parent = 0);

        /**
         * @brief ConnectionSettings, that was selected after pressing on
         * "Connect" button
         */
        ConnectionSettings *selectedConnection() const { return _selectedConnection; }

        /**
         * @brief This function is called when user clicks on "Connect" button.
         */
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
        /**
         * @brief ConnectionSettings, that was selected after pressing on
         * "Connect" button
         */
        ConnectionSettings *_selectedConnection;

        /**
         * @brief Main list widget
         */
        QTreeWidget *_listWidget;

        /**
         * @brief Settings manager
         */
        SettingsManager *_settingsManager;

        /**
         * @brief Hash that helps to connect ConnectionSettings with
         * ConnectionListWidgetItem*
         */
        ConnectionListItemContainerType _connectionItems;
    };    

    class ConnectionsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        ConnectionsTreeWidget();

    Q_SIGNALS:
        void layoutChanged();

    protected:
        void dropEvent(QDropEvent *event);
    };
}
