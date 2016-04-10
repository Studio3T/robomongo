#include "robomongo/gui/dialogs/CreateUserDialog.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QComboBox>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/domain/MongoUtils.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{
    const char * rolesText[CreateUserDialog::RolesCount] = {
        "read",
        "readWrite",
        "dbAdmin",
        "userAdmin",
        "clusterAdmin",
        "readAnyDatabase",
        "readWriteAnyDatabase",
        "userAdminAnyDatabase",
        "dbAdminAnyDatabase"
    };

    const QSize CreateUserDialog::minimumSize = QSize(400, 200);

    bool containsWord(const std::string& sentence, const std::string& word)
    {
        size_t pos = 0;
        while ((pos = sentence.substr(pos).find(word)) != std::string::npos) {
            if (pos + word.size() < sentence.size()) {
                char c = sentence[pos + word.size()];
                bool isLastAlpha = isalpha(c);
                bool isFirstAlpha = false;
                if (pos) {
                    isFirstAlpha = isalpha(sentence[pos - 1]);
                }
                if (!isFirstAlpha && !isLastAlpha)
                    return true;
            }
            pos++;
        }
        return false;
    }

    CreateUserDialog::CreateUserDialog(const QStringList &databases, const QString &serverName,
                                       const QString &database, const MongoUser &user,
                                       QWidget *parent) : QDialog(parent),
        _user(user)
    {
        setWindowTitle("Add User");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumSize(minimumSize);

        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);

        QFrame *hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        _userNameLabel = new QLabel("Name:");
        _userNameEdit = new QLineEdit();
        _userNameEdit->setText(QtUtils::toQString(user.name()));
        _userPassLabel = new QLabel("Password:");
        _userPassEdit = new QLineEdit();
        _userPassEdit->setEchoMode(QLineEdit::Password);
        _userSourceLabel = new QLabel("UserSource:");
        _userSourceComboBox = new QComboBox();
        _userSourceComboBox->addItems(QStringList() << "" << databases); //setText(QtUtils::toQString(user.userSource()));
        utils::setCurrentText(_userSourceComboBox, QtUtils::toQString(user.userSource()));

        QGridLayout *gridRoles = new QGridLayout();
        MongoUser::RoleType userRoles = user.role();
        for (unsigned i = 0; i<RolesCount; ++i)
        {
            int row = i%3;
            int col = i/3;
            _rolesArray[i] = new QCheckBox(rolesText[i], this);
            MongoUser::RoleType::const_iterator it = std::find(userRoles.begin(), userRoles.end(), rolesText[i]);
            _rolesArray[i]->setChecked(it!= userRoles.end());
            gridRoles->addWidget(_rolesArray[i], row, col);
        }

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->addStretch(1);
        hlayout->addWidget(buttonBox);

        QHBoxLayout *vlayout = new QHBoxLayout();
        vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        vlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
        vlayout->addStretch(1);

        QGridLayout *namelayout = new QGridLayout();
        namelayout->setContentsMargins(0, 7, 0, 7);
        namelayout->addWidget(_userNameLabel, 0, 0);
        namelayout->addWidget(_userNameEdit,  0, 1);
        namelayout->addWidget(_userPassLabel, 1, 0);
        namelayout->addWidget(_userPassEdit,  1, 1);
        namelayout->addWidget(_userSourceLabel, 2, 0);
        namelayout->addWidget(_userSourceComboBox,  2, 1);
        namelayout->addLayout(gridRoles,  3, 1);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(vlayout);
        layout->addWidget(hline);
        layout->addLayout(namelayout);
        layout->addLayout(hlayout);
        setLayout(layout);

        _userNameEdit->setFocus();
    }

    CreateUserDialog::CreateUserDialog(const QString &serverName,
        const QString &database, const MongoUser &user,
        QWidget *parent) : QDialog(parent),
        _user(user)
    {
        setWindowTitle("Add User");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumSize(minimumSize);

        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);

        QFrame *hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        _userNameLabel = new QLabel("Name:");
        _userNameEdit = new QLineEdit();
        _userNameEdit->setText(QtUtils::toQString(user.name()));
        _userPassLabel= new QLabel("Password:");
        _userPassEdit = new QLineEdit();
        _userPassEdit->setEchoMode(QLineEdit::Password);
        _readOnlyCheckBox = new QCheckBox("Read Only");
        _readOnlyCheckBox->setChecked(user.readOnly());

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->addStretch(1);
        hlayout->addWidget(buttonBox);

        QHBoxLayout *vlayout = new QHBoxLayout();
        vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        vlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
        vlayout->addStretch(1);

        QGridLayout *namelayout = new QGridLayout();
        namelayout->setContentsMargins(0, 7, 0, 7);
        namelayout->addWidget(_userNameLabel, 0, 0);
        namelayout->addWidget(_userNameEdit, 0, 1);
        namelayout->addWidget(_userPassLabel, 1, 0);
        namelayout->addWidget(_userPassEdit, 1, 1);
        namelayout->addWidget(_readOnlyCheckBox, 2, 1);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(vlayout);
        layout->addWidget(hline);
        layout->addLayout(namelayout);
        layout->addLayout(hlayout);
        setLayout(layout);

        _userNameEdit->setFocus();
    }

    void CreateUserDialog::setUserPasswordLabelText(const QString &text)
    {
        _userPassLabel->setText(text);
    }

    void CreateUserDialog::accept()
    {
        std::string username = QtUtils::toStdString(_userNameEdit->text());
        std::string pass = QtUtils::toStdString(_userPassEdit->text());
        if (_user.version() < MongoUser::minimumSupportedVersion) {
            if (username.empty() || pass.empty())
                return;

            std::string hash = MongoUtils::buildPasswordHash(username, pass);

            _user.setName(username);
            _user.setPasswordHash(hash);
            _user.setReadOnly(_readOnlyCheckBox->isChecked());
        }
        else {
            std::string userSource = QtUtils::toStdString(_userSourceComboBox->currentText());

            if (username.empty())
                return;

            if (userSource.empty() && pass.empty())
                return;

            if (!userSource.empty() && !pass.empty()) {
                QMessageBox::warning(this, "Invalid input", "The UserSource field and the Password field are mutually exclusive. The document cannot contain both.\n");
                return;
            }

            std::string hash;
            if (!pass.empty()) {
                hash = MongoUtils::buildPasswordHash(username, pass);
            }

            _user.setPasswordHash(hash);
            _user.setName(username);        
            _user.setUserSource(userSource);

            MongoUser::RoleType roles;
            for (unsigned i = 0; i < RolesCount; ++i) {
                if (_rolesArray[i]->isChecked()) {
                    roles.push_back(rolesText[i]);
                }
            }
            _user.setRole(roles);
        }

        QDialog::accept();
    }
    
}
