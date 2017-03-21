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

    class ConnectionBasicTab : public QWidget
    {
        Q_OBJECT

    public:
        ConnectionBasicTab(ConnectionSettings *settings);
        bool accept();

    private Q_SLOTS:
        void on_ConnectionTypeChange(int index);
        void deleteItem();
        void on_addButton_clicked();
        void on_removeButton_clicked();
        void on_replicaMemberItemEdit(QTreeWidgetItem* item, int column);

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
        QPushButton *_discoverButton;
        QLabel *_readPrefLabel;
        QComboBox *_readPreference;

        ConnectionSettings *const _settings;
    };
}
