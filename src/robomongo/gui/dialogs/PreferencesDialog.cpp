#include "robomongo/gui/dialogs/PreferencesDialog.h"

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/AppStyle.h"
#include "robomongo/gui/utils/ComboBoxUtils.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"

namespace Robomongo
{
    PreferencesDialog::PreferencesDialog(QWidget *parent)
        : BaseClass(parent)
    {
        setWindowIcon(GuiRegistry::instance().mainWindowIcon());

        setWindowTitle("Preferences " PROJECT_NAME_TITLE);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        setFixedSize(height, width);

        QVBoxLayout *layout = new QVBoxLayout(this);

        QHBoxLayout *defLayout = new QHBoxLayout(this);
        QLabel *defDisplayModeLabel = new QLabel("Default display mode:");
        defLayout->addWidget(defDisplayModeLabel);
        _defDisplayModeComboBox = new QComboBox();
        QStringList modes;
        for (int i = Text; i <= Custom; ++i)
        {
            modes.append(convertViewModeToString(static_cast<ViewMode>(i)));
        }
        _defDisplayModeComboBox->addItems(modes);
        defLayout->addWidget(_defDisplayModeComboBox);
        layout->addLayout(defLayout);

        QHBoxLayout *timeZoneLayout = new QHBoxLayout(this);
        QLabel *timeZoneLabel = new QLabel("Display Dates in:");
        timeZoneLayout->addWidget(timeZoneLabel);
        _timeZoneComboBox = new QComboBox();
        QStringList times;
        for (int i = Utc; i <= LocalTime; ++i)
        {
            times.append(convertTimesToString(static_cast<SupportedTimes>(i)));
        }
        _timeZoneComboBox->addItems(times);
        timeZoneLayout->addWidget(_timeZoneComboBox);
        layout->addLayout(timeZoneLayout);

        QHBoxLayout *uuidEncodingLayout = new QHBoxLayout(this);
        QLabel *uuidEncodingLabel = new QLabel("Legacy UUID Encoding:");
        uuidEncodingLayout->addWidget(uuidEncodingLabel);
        _uuidEncodingComboBox = new QComboBox();
        QStringList uuids;
        for (int i = DefaultEncoding; i <= PythonLegacy; ++i)
        {
            uuids.append(convertUUIDEncodingToString(static_cast<UUIDEncoding>(i)));
        }
        _uuidEncodingComboBox->addItems(uuids);
        uuidEncodingLayout->addWidget(_uuidEncodingComboBox);
        layout->addLayout(uuidEncodingLayout);        

        _loadMongoRcJsCheckBox = new QCheckBox("Load .mongorc.js");
        layout->addWidget(_loadMongoRcJsCheckBox);

        _disabelConnectionShortcutsCheckBox = new QCheckBox("Disable connection shortcuts");
        layout->addWidget(_disabelConnectionShortcutsCheckBox);

        QHBoxLayout *stylesLayout = new QHBoxLayout(this);
        QLabel *stylesLabel = new QLabel("Styles:");
        stylesLayout->addWidget(stylesLabel);
        _stylesComboBox = new QComboBox();
        _stylesComboBox->addItems(AppStyleUtils::getSupportedStyles());
        stylesLayout->addWidget(_stylesComboBox);
        layout->addLayout(stylesLayout);   

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));
        layout->addWidget(buttonBox);
        setLayout(layout);

        syncWithSettings();
    }

    void PreferencesDialog::syncWithSettings()
    {

        utils::setCurrentText(_defDisplayModeComboBox, convertViewModeToString(Robomongo::AppRegistry::instance().settingsManager()->viewMode()));
        utils::setCurrentText(_timeZoneComboBox, convertTimesToString(Robomongo::AppRegistry::instance().settingsManager()->timeZone()));
        utils::setCurrentText(_uuidEncodingComboBox, convertUUIDEncodingToString(Robomongo::AppRegistry::instance().settingsManager()->uuidEncoding()));
        _loadMongoRcJsCheckBox->setChecked(AppRegistry::instance().settingsManager()->loadMongoRcJs());
        _disabelConnectionShortcutsCheckBox->setChecked(AppRegistry::instance().settingsManager()->disableConnectionShortcuts());
        utils::setCurrentText(_stylesComboBox, Robomongo::AppRegistry::instance().settingsManager()->currentStyle());
    }

    void PreferencesDialog::accept()
    {
        ViewMode mode = convertStringToViewMode(QtUtils::toStdString(_defDisplayModeComboBox->currentText()).c_str());
        Robomongo::AppRegistry::instance().settingsManager()->setViewMode(mode);

        SupportedTimes time = convertStringToTimes(QtUtils::toStdString(_timeZoneComboBox->currentText()).c_str());
        Robomongo::AppRegistry::instance().settingsManager()->setTimeZone(time);

        UUIDEncoding uuidC = convertStringToUUIDEncoding(QtUtils::toStdString(_uuidEncodingComboBox->currentText()).c_str());
        Robomongo::AppRegistry::instance().settingsManager()->setUuidEncoding(uuidC);

        AppRegistry::instance().settingsManager()->setLoadMongoRcJs(_loadMongoRcJsCheckBox->isChecked());
        AppRegistry::instance().settingsManager()->setDisableConnectionShortcuts(_disabelConnectionShortcutsCheckBox->isChecked());
        Robomongo::AppRegistry::instance().settingsManager()->setCurrentStyle(_stylesComboBox->currentText());
        AppStyleUtils::applyStyle(_stylesComboBox->currentText());
        Robomongo::AppRegistry::instance().settingsManager()->save();

        return BaseClass::accept();
    }
}
