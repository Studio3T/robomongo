
#include "robomongo/gui/dialogs/ChangeShellTimeoutDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QObject>
#include <QLabel>
#include <QGridLayout>
#include <QIntValidator>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/SettingsManager.h"

namespace Robomongo
{
    void changeShellTimeoutDialog()
    {
        auto changeShellTimeoutDialog = new QDialog;
        auto settingsManager = AppRegistry::instance().settingsManager();
        auto currentShellTimeout = new QLabel(QString::number(settingsManager->shellTimeoutSec()));
        auto newShellTimeout = new QLineEdit;
        newShellTimeout->setValidator(new QIntValidator(0, 100000));
        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
        QObject::connect(buttonBox, SIGNAL(accepted()), changeShellTimeoutDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), changeShellTimeoutDialog, SLOT(reject()));
        auto lay = new QGridLayout;
        auto firstLabel = new QLabel("Enter new value for Robo 3T shell timeout in seconds:\n");
        lay->addWidget(firstLabel,                      0, 0, 1, 2, Qt::AlignLeft);
        lay->addWidget(new QLabel("Current Value: "),   1, 0);
        lay->addWidget(currentShellTimeout,             1, 1);
        lay->addWidget(new QLabel("New Value: "),       2, 0);
        lay->addWidget(newShellTimeout,                 2, 1);
        lay->addWidget(buttonBox,                       3, 0, 1, 2, Qt::AlignRight);
        changeShellTimeoutDialog->setLayout(lay);
        changeShellTimeoutDialog->setWindowTitle("Robo 3T");

        if (changeShellTimeoutDialog->exec()) {
            settingsManager->setShellTimeoutSec(newShellTimeout->text().toInt());
            settingsManager->save();
            auto subStr = settingsManager->shellTimeoutSec() > 1 ? " seconds." : " second.";
            LOG_MSG("Shell timeout value changed from " + currentShellTimeout->text() + " to " +
                QString::number(settingsManager->shellTimeoutSec()) + subStr, mongo::logger::LogSeverity::Info());

            for (auto server : AppRegistry::instance().app()->getServers())
                server->changeWorkerShellTimeout(std::abs(newShellTimeout->text().toInt()));
        }
    }
}
