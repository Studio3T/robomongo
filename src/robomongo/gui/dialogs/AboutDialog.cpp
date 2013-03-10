#include "AboutDialog.h"

#include <QDate>
#include <QFile>
#include <QSysInfo>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowIcon(GuiRegistry::instance().mainWindowIcon());

    setWindowTitle("About Robomongo");
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QGridLayout *layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    QString version = AppRegistry::instance().version();

     const QString description = tr(
        "<h3>Robomongo %1</h3>"
        "Shell-centric MongoDB management tool."
        "<br/>"
        "<br/>"
        "Visit Robomongo website: <a href=\"www.robomongo.org\">www.robomongo.org</a> <br/>"
        "<br/>"
        "<a href=\"https://github.com/paralect/robomongo\">Fork</a> project or <a href=\"https://github.com/paralect/robomongo/issues\">submit</a> issues/proposals on GitHub.  <br/>"
        "<br/>"
        "Copyright 2013 <a href=\"http://www.paralect.com\">Paralect</a>. All rights reserved.<br/>"
        "<br/>"
        "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
        "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
        "PARTICULAR PURPOSE.<br/>")
        .arg(version);

    QLabel *copyRightLabel = new QLabel(description);
    copyRightLabel->setWordWrap(true);
    copyRightLabel->setOpenExternalLinks(true);
    copyRightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
    connect(buttonBox , SIGNAL(rejected()), this, SLOT(reject()));

    QIcon icon = GuiRegistry::instance().mainWindowIcon();
    QPixmap iconPixmap = icon.pixmap(48, 48);


    QLabel *logoLabel = new QLabel;
    logoLabel->setPixmap(iconPixmap);
    layout->addWidget(logoLabel , 0, 0, 1, 1);
    layout->addWidget(copyRightLabel, 0, 1, 4, 4);
    layout->addWidget(buttonBox, 4, 0, 1, 5);
}
