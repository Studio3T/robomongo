#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>

#include "robomongo/core/domain/MongoUser.h"

namespace Robomongo
{
    class CreateUserDialog : public QDialog
    {
        Q_OBJECT

    public:
        CreateUserDialog(const QString &serverName,
                         const QString &database = QString(),
                         const MongoUser &user = MongoUser(),
                         QWidget *parent = 0);

        MongoUser user() const { return _user; }
        void setUserPasswordLabelText(const QString &text);

    public slots:
        virtual void accept();

    private:
        MongoUser _user;

        QLabel *_userNameLabel;
        QLineEdit *_userNameEdit;
        QLabel *_userPassLabel;
        QLineEdit *_userPassEdit;
        QCheckBox *_readOnlyCheckBox;
    };
}

