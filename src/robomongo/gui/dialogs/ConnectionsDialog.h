#ifndef CONNECTIONSDIALOG_H
#define CONNECTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QListWidgetItem>
#include <QHash>

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

        /**
         * @brief Creates dialog
         */
        ConnectionsDialog(SettingsManager *manager);

        /**
         * @brief ConnectionSettings, that was selected after pressing on
         * "Connect" button
         */
        ConnectionSettings *selectedConnection() const { return _selectedConnection; }

        /**
         * @brief This function is called when user clicks on "Connect" button.
         */
        virtual void accept();

    private slots:

        void linkActivated(const QString &link);

        /**
         * @brief Add connection to the list widget
         */
        void add(ConnectionSettings *connection);

        /**
         * @brief Update specified connection (if it exists for this dialog)
         */
        void update(ConnectionSettings *connection);

        /**
         * @brief Remove specified connection (if it exists for this dialog)
         */
        void remove(ConnectionSettings *connection);

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
        QHash<ConnectionSettings *, ConnectionListWidgetItem *> _hash;
    };

    /**
     * @brief Simple ListWidgetItem that has several convenience methods.
     */
    class ConnectionListWidgetItem : public QTreeWidgetItem
    {
    public:

        /**
         * @brief Creates ConnectionListWidgetItem with specified ConnectionSettings
         */
        ConnectionListWidgetItem(ConnectionSettings *connection) { setConnection(connection); }

        /**
         * @brief Returns attached ConnectionSettings.
         */
        ConnectionSettings *connection() { return _connection; }

        /**
         * @brief Attach ConnectionSettings to this item
         */
        void setConnection(ConnectionSettings *connection);

    private:
        ConnectionSettings *_connection;
    };

    class ConnectionsTreeWidget : public QTreeWidget
    {
        Q_OBJECT
    public:
        ConnectionsTreeWidget();

    signals:
        void layoutChanged();

    protected:
//        void dragMoveEvent(QDragMoveEvent * event);
//        void dragEnterEvent(QDragEnterEvent * event);
        void dropEvent(QDropEvent * event);
//        void mousePressEvent(QMouseEvent *event);
    private:
        QTreeWidgetItem *draggingItem;
    };
}


#endif // CONNECTIONSDIALOG_H
