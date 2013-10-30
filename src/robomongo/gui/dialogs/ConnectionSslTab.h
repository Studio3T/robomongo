#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;

    class ConnectionSslTab : public QWidget
    {
        Q_OBJECT

    public:
        ConnectionSslTab(ConnectionSettings *settings);
        void accept();
        bool isSslSupported() const;

    private Q_SLOTS:
        void setSslPEMKeyFile();
        void sslSupportStateChange(int val);

    private:
        QCheckBox *_sslSupport;
        QLineEdit *_sslPemFilePath;
        QLabel    *_sslPemLabel;
        QLabel    *_sslPemDescriptionLabel;
        QPushButton *_selectPemFileButton;

        ConnectionSettings *const _settings;        
    };
}
