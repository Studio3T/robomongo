#ifndef EDITCONNECTIONDIALOG_H
#define EDITCONNECTIONDIALOG_H

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <settings/ConnectionRecord.h>

namespace Robomongo
{
    /*
    ** Dialog allows to edit connection
    */
    class EditConnectionDialog : public QDialog
    {
        Q_OBJECT

    private:

        /*
        ** Text boxs
        */
        QLineEdit * _connectionName;
        QLineEdit * _serverAddress;
        QLineEdit * _serverPort;
        QLineEdit * _userName;
        QLineEdit * _userPassword;
        QLineEdit * _databases;

        /*
        ** View model
        */
        ConnectionRecord _connection;

        /*
        ** Check that user is okay to close this window
        */
        bool _canBeClosed();

    protected:

        /*
        ** Close event handler
        */
        void closeEvent(QCloseEvent *);

        void keyPressEvent(QKeyEvent *e);


    protected slots:

        /*
        ** Handles moment when close button was clicked
        */
        void ui_closeButtonClicked();

        /*
        ** Handle test button clicked event
        */
        void ui_testButtonClicked();

    public:

        /*
        ** Constructs dialog with specified viewmodel
        */
        EditConnectionDialog(ConnectionRecord connection);

        /*
        ** Destructs dialog
        */
        ~EditConnectionDialog();

        /*
        ** Returns current state of view model
        */
        ConnectionRecord connection() { return _connection; }

        /*
        ** Override virtual method
        */
        virtual void accept();
    };
}


#endif // EDITCONNECTIONDIALOG_H
