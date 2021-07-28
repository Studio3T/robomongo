#pragma once

#include <QWidget>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
class QCheckBox;
class QPushButton;
class QComboBox;
class QTreeWidget;
class QTreeWidgetItem;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class ConnectionSettings;
    class ConnectionDialog;

    class ConnectionBasicTab : public QWidget
    {
        Q_OBJECT

    public:
        ConnectionBasicTab(ConnectionSettings *settings, ConnectionDialog *connectionDialog);
        bool accept();
        void clearTab();

    private Q_SLOTS:
        void on_ConnectionTypeChange(int index);
        void deleteItem();
        void on_addButton_clicked();
        void on_removeButton_clicked();
        void on_replicaMemberItemEdit(QTreeWidgetItem* item, int column);
        void on_uriButton_clicked();

    private:
        QLabel *_typeLabel;
        QComboBox *_connectionType;
        QLabel *_nameLabel;
        QLineEdit *_connectionName;
        QLabel *_connInfoLabel;
        QLabel *_addressLabel;
        QLineEdit *_serverAddress;
        QLabel *_colon;
        QLineEdit *_serverPort;
        QLabel *_addInfoLabel;
        QLabel *_membersLabel;
        QTreeWidget *_members;
        QPushButton *_addButton;
        QPushButton *_removeButton;
        QDialogButtonBox *_minusPlusButtonBox;
        QLabel *_setNameLabel;
        QLineEdit *_setNameEdit;
        QLineEdit *_uriEdit;
        QPushButton *_uriButton;
        QPushButton *_discoverButton;
        QLabel *_readPrefLabel;
        QComboBox *_readPreference;

        ConnectionSettings *const _settings;
        ConnectionDialog *_connectionDialog;
    };
}
