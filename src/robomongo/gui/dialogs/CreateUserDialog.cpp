#include "robomongo/gui/dialogs/CreateUserDialog.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/core/domain/MongoUtils.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

CreateUserDialog::CreateUserDialog(const QString &serverName,
                                   const QString &database, const MongoUser &user,
                                   QWidget *parent) : QDialog(parent),
    _user(user)
{
    setWindowTitle("Add User");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
    setMinimumWidth(400);

    Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);
    Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);

    QFrame *hline = new QFrame();
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);

    _userNameLabel= new QLabel("Name:");
    _userNameEdit = new QLineEdit();
    _userNameEdit->setText(user.name());
    _userPassLabel= new QLabel("Password:");
    _userPassEdit = new QLineEdit();
    _readOnlyCheckBox = new QCheckBox("Read Only");
    _readOnlyCheckBox->setChecked(user.readOnly());

    QPushButton *cancelButton = new QPushButton("&Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    QPushButton *_okButton = new QPushButton("&Create");
    connect(_okButton, SIGNAL(clicked()), this, SLOT(accept()));

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->addStretch(1);

#if defined(Q_OS_MAC)
    hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
    hlayout->addWidget(_okButton, 0, Qt::AlignRight);
#else
    hlayout->addWidget(_okButton, 0, Qt::AlignRight);
    hlayout->addWidget(cancelButton, 0, Qt::AlignRight);
#endif

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
    namelayout->addWidget(_readOnlyCheckBox,  2, 1);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(vlayout);
    layout->addWidget(hline);
    layout->addLayout(namelayout);
    layout->addLayout(hlayout);
    setLayout(layout);
}

void CreateUserDialog::setUserPasswordLabelText(const QString &text)
{
    _userPassLabel->setText(text);
}

void CreateUserDialog::accept()
{
    QString username = _userNameEdit->text();
    QString pass = _userPassEdit->text();

    if (username.isEmpty() || pass.isEmpty())
        return;

    QString hash = MongoUtils::buildPasswordHash(username, pass);

    _user.setName(username);
    _user.setPasswordHash(hash);
    _user.setReadOnly(_readOnlyCheckBox->isChecked());

    QDialog::accept();
}
