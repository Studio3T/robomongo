#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QTextEdit;
class QLabel;
class QCheckBox;
class QLineEdit;
QT_END_NAMESPACE

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
        enum { RolesCount = 9};

    public Q_SLOTS:
        virtual void accept();

    private:
        MongoUser _user;

        QLabel *_userNameLabel;
        QLineEdit *_userNameEdit;
        QLabel *_userPassLabel;
        QLineEdit *_userPassEdit;

        QCheckBox *_rolesArray[RolesCount];
    };
}

