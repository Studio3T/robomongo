#pragma once

#include <QDialog>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QLabel;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidgetItem;
class QTreeWidget;
class QComboBox;
class QPushButton;
class QGroupBox;
class QTextEdit;
class QProcess;
QT_END_NAMESPACE

namespace Robomongo
{
    class Indicator;

    /**
    * @brief This class is not finished, it is still under development.
    *        Currently export is disabled in the GUI. If enabled in GUI, export 
    *        will work with current code. However, the code is unfinished; 
    *        it needs to finished, reviewed and tested.
    *        Currently, there is support for simultaneous export which can be done 
    *        on multiple export dialogs without blocking each other or main window. 
    *        So user can still work while multiple export dialogs are doing export. 
    *        This achieved by the assumption that there will be usable export executable for 
    *        Robomongo to use. Currently, this location is hard coded as 
    *        "D:\\mongo_export\\bin\\mongoexport.exe". One of the next actions will be 
    *        to design the location of mongoexport which should be cross-platform, 
    *        generic and not hard coded.
    */
    class ExportDialog : public QDialog
    {
        Q_OBJECT

        enum MODE 
        {
            AUTO = 0,
            MANUAL = 1
        };

    public:
        explicit ExportDialog(QString const& dbName, QString const& collName, QWidget *parent = 0);
        void setOkButtonText(const QString &text);
        void setInputLabelText(const QString &text);
        void setInputText(const QString &text);
        enum { maxLenghtName = 60 };

    public Q_SLOTS:
        virtual void accept();

    private Q_SLOTS:
        void ui_itemExpanded(QTreeWidgetItem *item);
        void ui_itemDoubleClicked(QTreeWidgetItem *item, int column);
        void on_browseButton_clicked();
        void on_formatComboBox_change(int index);
        void on_modeButton_clicked();
        void on_exportFinished(int exitCode, QProcess::ExitStatus exitStatus);
        void on_processErrorOccurred(QProcess::ProcessError);
        void on_viewOutputLink(QString);

    private:
        // todo: remove if unused
        Indicator *createDatabaseIndicator(const QString &database);
        Indicator *createCollectionIndicator(const QString &collection);

        // Enable/Disable widgets during/after export operation
        // @param enable: true to enable, false to disable widgets
        void enableDisableWidgets(bool enable) const;

        // Auto Mode
        QGroupBox* _inputsGroupBox;
        QComboBox* _formatComboBox;
        QLabel* _fieldsLabel;
        QLineEdit* _fields;
        QLineEdit* _query;
        QLineEdit* _outputFileName;
        QLineEdit* _outputDir;
        QPushButton* _browseButton;
        QGroupBox* _autoOutputsGroup;
        QTextEdit* _exportOutput;
        QLabel* _viewOutputLink;
        //
        QTextEdit* _mongoExportOutput;

        // Manual Mode
        QTextEdit* _manualExportCmd;
        QPushButton* _modeButton;
        QGroupBox* _manualGroupBox;
        // Common
        QDialogButtonBox* _buttonBox;

        MODE _mode;
        QString _dbName;
        QString _collName;
        QString _mongoExportArgs;
        QString _exportResult;
        QString _mongoExportOutputStr;
        QProcess* _activeProcess;          // pointer to running/finished process
    };
}
