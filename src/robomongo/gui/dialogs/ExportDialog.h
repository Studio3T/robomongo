#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidgetItem;
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
        void ui_itemClicked(QTreeWidgetItem *current, QTreeWidgetItem *previous);
        void on_browseButton_clicked();

    private:
        // todo: remove
        Indicator *createDatabaseIndicator(const QString &database);
        Indicator *createCollectionIndicator(const QString &collection);

        QComboBox* _formatComboBox;
        QLineEdit* _outputFileName;
        QLineEdit* _outputDir;
        QDialogButtonBox* _buttonBox;

        QString _mongoExportArgs;
    };
}
