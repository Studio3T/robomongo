#ifndef EDITCONNECTIONDIALOG_H
#define EDITCONNECTIONDIALOG_H

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <settings/ConnectionRecord.h>
#include "Core.h"

namespace Robomongo
{
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
        EditConnectionDialog(ConnectionRecordPtr connection);

        /**
         * @brief Accept() is called when user agree with entered data.
         */
        virtual void accept();

    protected:

        /**
         * @brief Close event handler
         */
        void closeEvent(QCloseEvent *);

        /**
         * @brief Key press event handler
         */
        void keyPressEvent(QKeyEvent *e);


    private slots:

        /**
         * @brief Test current connection
         */
        void testConnection();

    private:

        /**
         * @brief Text boxes
         */
        QLineEdit *_connectionName;
        QLineEdit *_serverAddress;
        QLineEdit *_serverPort;
        QLineEdit *_userName;
        QLineEdit *_userPassword;
        QLineEdit *_databaseName;

        /**
         * @brief Edited connection
         */
        ConnectionRecordPtr _connection;

        /**
         * @brief Check that it is okay to close this window
         *        (there is no modification of data, that we possibly can loose)
         */
        bool canBeClosed();
    };
}


#endif // EDITCONNECTIONDIALOG_H
