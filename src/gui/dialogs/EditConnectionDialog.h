#ifndef EDITCONNECTIONDIALOG_H
#define EDITCONNECTIONDIALOG_H

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <settings/ConnectionSettings.h>
#include "Core.h"
#include <QTreeWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QCheckBox>
#include <QLabel>
#include <QGridLayout>

namespace Robomongo
{
    class CredentialModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:

        CredentialModel(QList<CredentialSettings *> credentials) : _credentials(credentials) {}

        int rowCount(const QModelIndex &parent) const;
        int columnCount(const QModelIndex &parent) const;
        QVariant data(const QModelIndex &index, int role) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);
        Qt::ItemFlags flags(const QModelIndex &index) const;

        void updateAll();
        void remove(int at);

    private:
        QList<CredentialSettings *> _credentials;
    };

    class AuthWidget : public QWidget
    {
        Q_OBJECT
    public:
        AuthWidget();
        QLineEdit *_userName;
        QLabel    *_userNameLabel;
        QLineEdit *_userPassword;
        QLabel    *_userPasswordLabel;
        QLineEdit *_databaseName;
        QLabel    *_databaseNameLabel;
        QLabel    *_databaseNameDescriptionLabel;
        QCheckBox *_useAuth;
        //QGridLayout *_authLayout;

    protected slots:
        void authChecked(bool checked);
    };

    class ServerWidget : public QWidget
    {
        Q_OBJECT
    public:
        ServerWidget();
        QLineEdit *_connectionName;
        QLineEdit *_serverAddress;
        QLineEdit *_serverPort;
        QLineEdit *_defaultDatabaseName;
        //QGridLayout *_authLayout;
    };

    class AdvancedWidget : public QWidget
    {
        Q_OBJECT
    public:
        AdvancedWidget();
        QLineEdit *_defaultDatabaseName;
        //QGridLayout *_authLayout;
    };


    /**
     * @brief This Dialog allows to edit single connection
     */
    class EditConnectionDialog : public QDialog
    {
        Q_OBJECT

    public:

        /**
         * @brief Constructs dialog with specified connection
         */
        EditConnectionDialog(ConnectionSettings *connection);

        /**
         * @brief Accept() is called when user agree with entered data.
         */
        virtual void accept();

        void addCredential(CredentialSettings *credential);

    protected:

        /**
         * @brief Close event handler
         */
        void closeEvent(QCloseEvent *);

        /**
         * @brief Key press event handler
         */
        void keyPressEvent(QKeyEvent *e);

        void showEvent(QShowEvent *e);


    private slots:

        /**
         * @brief Test current connection
         */
        void testConnection();

        void deleteCredential();

        void authChecked(bool checked);

        void tabWidget_currentChanged(int index);

    private:

        void updateCredentialTree();

        /**
         * @brief Text boxes
         */
        QLineEdit *_connectionName;
        QLineEdit *_serverAddress;
        QLineEdit *_serverPort;
        QLineEdit *_defaultDatabaseName;

        QLineEdit *_userName;
        QLabel    *_userNameLabel;
        QLineEdit *_userPassword;
        QLineEdit *_databaseName;

        QCheckBox *_useAuth;
        QGridLayout *_authLayout;

        AuthWidget *_auth;
        ServerWidget *_serverTab;
        AdvancedWidget *_advancedTab;

        QTreeWidget *_credentialsTree;

        QTableView *_credentialsView;
        CredentialModel *_credentialsModel;
        QAbstractItemModel *_hdh;

        QTabWidget *_tabWidget;

        /**
         * @brief Edited connection
         */
        ConnectionSettings *_connection;

        /**
         * @brief Check that it is okay to close this window
         *        (there is no modification of data, that we possibly can loose)
         */
        bool canBeClosed();
    };
}


#endif // EDITCONNECTIONDIALOG_H
