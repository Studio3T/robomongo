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
        ConnectionsDialog(SettingsManager *manager, bool checkForImported, QWidget *parent = 0);

        /**
         * @brief ConnectionSettings, that was selected after pressing on
         * "Connect" button
         */
        ConnectionSettings *selectedConnection() const { return _selectedConnection; }

    public Q_SLOTS:
        /**
         * @brief This function is called when user clicks on "Connect" button.
         */
        void accept() override;

        /**
        * @brief Called when "Cancel" button clicked.
        */
        void reject() override;

        /**
        * @brief Add connection to the list widget
        */
        void add(ConnectionSettings *connection);
        
    protected:
        /**
        * @brief Reimplementing closeEvent in order to do some pre-close actions.
        */
        void closeEvent(QCloseEvent *event) override;

    private Q_SLOTS:
        void linkActivated(const QString &link);

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

        void keyPressEvent(QKeyEvent* event) override;

    private:

        /**
        * @brief Restore window settings from system registry
        */
        void restoreWindowSettings();

        /**
        * @brief Save windows settings into system registry
        */
        void saveWindowSettings() const;

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

        bool _checkForImported;
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
