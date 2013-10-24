#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QTextEdit;
class QLabel;
class QCheckBox;
class QLineEdit;
class QComboBox;
QT_END_NAMESPACE

#include "robomongo/core/domain/MongoUser.h"

namespace Robomongo
{
    class CreateUserDialog : public QDialog
    {
        Q_OBJECT

    public:
        static const QSize minimumSize;

        CreateUserDialog(const QStringList &databases, const QString &serverName,
                         const QString &database,
                         const MongoUser &user,
                         QWidget *parent = 0);

        CreateUserDialog(const QString &serverName,
            const QString &database,
            const MongoUser &user,
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
        QLabel *_userSourceLabel;
        QComboBox *_userSourceComboBox;
        QCheckBox *_readOnlyCheckBox;
        QCheckBox *_rolesArray[RolesCount];
    };
}

