#include "robomongo/gui/dialogs/SSLTab.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QRegExpValidator>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QComboBox>
#include <QMessageBox>
#include <QFileInfo>
#include <QFrame>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"

namespace {
    const QString askPasswordText = "Ask for password each time";
    const QString askPassphraseText = "Ask for passphrase each time";
    bool isFileExists(const QString &path) {
        QFileInfo fileInfo(path);
        return fileInfo.exists() && fileInfo.isFile();
    }
}

namespace Robomongo
{
    SSLTab::SSLTab(ConnectionSettings *settings) 
        //: _settings(settings)
    {

    }

}