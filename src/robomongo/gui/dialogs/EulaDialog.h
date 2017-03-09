#pragma once

#include <QWizard>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QDialogButtonBox;
class QComboBox;
class QNetworkReply;
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
        void closeEvent(QCloseEvent *event) override;

    private Q_SLOTS:
        void on_agreeButton_clicked();
        void on_notAgreeButton_clicked();
        void on_next_clicked();
        void on_back_clicked();
        void on_finish_clicked();

    private:

        // Send name, last name and email data of user to home server
        void postUserData() const;

        /**
        * @brief Restore window settings from system registry
        */
        void restoreWindowSettings();

        /**
        * @brief Save window settings into system registry
        */
        void saveWindowSettings() const;

        QLineEdit* _nameEdit;
        QLineEdit* _lastNameEdit;
        QLineEdit* _emailEdit;

        QByteArray _postData;
        mutable QNetworkReply* _reply;

    };
}

