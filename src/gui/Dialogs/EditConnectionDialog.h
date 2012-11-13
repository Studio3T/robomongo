#ifndef EDITCONNECTIONDIALOG_H
#define EDITCONNECTIONDIALOG_H

#include <QDialog>

class ConnectionDialogViewModel;

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
    ConnectionDialogViewModel * _connection;

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
    EditConnectionDialog(ConnectionDialogViewModel * connection);

    /*
    ** Destructs dialog
    */
    ~EditConnectionDialog();

    /*
    ** Returns current state of view model
    */
    ConnectionDialogViewModel * connection() { return _connection; }

    /*
    ** Override virtual method
    */
    virtual void accept();
};

#endif // EDITCONNECTIONDIALOG_H
