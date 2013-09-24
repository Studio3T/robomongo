#pragma  once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QComboBox;
class QCheckBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class PreferencesDialog : public QDialog
    {
        Q_OBJECT

    public:
        typedef QDialog BaseClass;
        explicit PreferencesDialog(QWidget *parent);
        enum { height = 640, width = 480};
    public Q_SLOTS:
        virtual void accept();
    private:
        void syncWithSettings();
    private:
        QComboBox *_defDisplayModeComboBox;
        QComboBox *_timeZoneComboBox;
        QComboBox *_uuidEncodingComboBox;
        QCheckBox *_loadMongoRcJsCheckBox;
        QCheckBox *_disabelConnectionShortcutsCheckBox;
        QComboBox *_stylesComboBox;
    };
}
