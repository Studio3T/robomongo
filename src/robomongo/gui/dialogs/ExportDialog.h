#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidgetItem;
class QTreeWidget;
class QComboBox;
QT_END_NAMESPACE

namespace Robomongo
{
    class Indicator;

    class ExportDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit ExportDialog(QWidget *parent = 0);
        //QString databaseName() const;
        void setOkButtonText(const QString &text);
        void setInputLabelText(const QString &text);
        void setInputText(const QString &text);
        enum { maxLenghtName = 60 };
        const static QSize dialogSize;

    public Q_SLOTS:
        virtual void accept();

    private Q_SLOTS:
        void ui_itemExpanded(QTreeWidgetItem *item);
        void ui_itemDoubleClicked(QTreeWidgetItem *item, int column);
        void ui_itemClicked(QTreeWidgetItem *current);
        void on_browseButton_clicked();
        void on_formatComboBox_change(int index);

    private:
        // todo: remove
        Indicator *createDatabaseIndicator(const QString &database);
        Indicator *createCollectionIndicator(const QString &collection);

        QComboBox* _formatComboBox;
        QLabel* _fieldsLabel;
        QLineEdit* _fields;
        QLineEdit* _query;
        QLineEdit* _outputFileName;
        QLineEdit* _outputDir;
        QDialogButtonBox* _buttonBox;
        QTreeWidget* _treeWidget;

        QString _dbName;
        QString _collName;
        QString _mongoExportArgs;
    };
}
