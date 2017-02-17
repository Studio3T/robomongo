#pragma once

#include <QWizard>

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

    class EulaDialog : public QWizard
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
        void on_agreeButton_clicked();
        void on_notAgreeButton_clicked();
        void on_next_clicked();
        void on_back_clicked();
        void on_finish_clicked();

    private:

        /**
        * @brief Restore window settings from system registry
        */
        void restoreWindowSettings();

        /**
        * @brief Save window settings into system registry
        */
        void saveWindowSettings() const;

    };
}

