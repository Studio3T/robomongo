#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QDialogButtonBox;
class QComboBox;
QT_END_NAMESPACE

namespace Robomongo
{

    class EulaDialog : public QDialog
    {
        Q_OBJECT

    public:
        static const QSize minimumSize;

        explicit EulaDialog(QWidget *parent = nullptr);
        
    public Q_SLOTS:
        void accept() override;
        void reject() override;

    protected:
        /**
        * @brief Reimplementing closeEvent in order to do some pre-close actions.
        */
        void closeEvent(QCloseEvent *event) override;


    private Q_SLOTS:
        void enableOkButton(int state);

    private:

        /**
        * @brief Restore window settings from system registry
        */
        void restoreWindowSettings();

        /**
        * @brief Save window settings into system registry
        */
        void saveWindowSettings() const;

        
        QCheckBox* _checkBox = nullptr;
        QDialogButtonBox* _buttonBox;

    };
}

