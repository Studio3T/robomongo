#include "robomongo/gui/dialogs/ExportDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QApplication>
#include <QProcess>
#include <QDir>
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QFileInfo>
#include <QSizePolicy>
#include <QProgressDialog>

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/widgets/explorer/ExplorerWidget.h"
#include "robomongo/gui/widgets/explorer/ExplorerTreeWidget.h"
#include "robomongo/gui/widgets/explorer/ExplorerServerTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerCollectionTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseCategoryTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerDatabaseTreeItem.h"
#include "robomongo/gui/widgets/explorer/ExplorerReplicaSetTreeItem.h"
#include "robomongo/gui/utils/GuiConstants.h"
#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{
    namespace
    {
        const QString defaultDir = "D:\\exports\\";     // Default location

        auto const AUTO_MODE_SIZE = QSize(500, 450);
        auto const MANUAL_MODE_SIZE = QSize(500, 400);

        auto const SHOW_DETAILS = "<a href='error' style='color: #777777;'>Show details</a>";
        auto const HIDE_DETAILS = "<a href='error' style='color: #777777;'>Hide details</a>";

        // This structure represents the arguments as in "mongoexport.exe --help"
        // See http://docs.mongodb.org/manual/reference/program/mongoexport/ for more information
        struct MongoExportArgs
        {
            static QString db(const QString& dbName) { return ("--db" + dbName); }
            static QString collection(const QString& collection) { return ("--collection" + collection); }
            
            // i.e. absFilePath: "/exports/coll1.json"
            static QString out(const QString& absFilePath) { return ("--out " + absFilePath); }  
        };
    }

    ExportDialog::ExportDialog(QString const& dbName, QString const& collName, QWidget *parent) :
        QDialog(parent), _mode(AUTO), _mongoExportArgs(), _activeProcess(nullptr)
    {
        setWindowTitle("Export Collection");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumSize(AUTO_MODE_SIZE);
        //setFixedHeight(AUTO_MODE_SIZE.height());

        _activeProcess = new QProcess(this);
        //_activeProcess->setProcessChannelMode(QProcess::MergedChannels);
        VERIFY(connect(_activeProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(on_exportFinished(int, QProcess::ExitStatus))));
        VERIFY(connect(_activeProcess, SIGNAL(errorOccurred(QProcess::ProcessError)),
            this, SLOT(on_processErrorOccurred(QProcess::ProcessError))));

        // todo: move to a global location
        // Enable copyable text for QMessageBox
        qApp->setStyleSheet("QMessageBox { messagebox-text-interaction-flags: 5; }");

        const QString serverName = "localhost:20017"; // todo: remove
        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);

        // Horizontal line
        QFrame *horline = new QFrame();
        horline->setFrameShape(QFrame::HLine);
        horline->setFrameShadow(QFrame::Sunken);

        // Widgets related to Input
        auto dbNameLabel = new QLabel("Database Name:");
        auto dbNameLineEdit = new QLineEdit;
        auto dbNameLay = new QHBoxLayout;
        dbNameLay->addWidget(dbNameLabel);
        dbNameLay->addWidget(dbNameLineEdit);

        auto collNameLabel = new QLabel("Collection Name:");
        auto collNameLineEdit = new QLineEdit;
        auto collNameLay = new QHBoxLayout;
        collNameLay->addWidget(collNameLabel);
        collNameLay->addWidget(collNameLineEdit);

        auto selectedCollLay = new QGridLayout;
        selectedCollLay->setAlignment(Qt::AlignTop);
        selectedCollLay->setColumnStretch(2, 1);

        auto serverIcon = new QLabel("<html><img src=':/robomongo/icons/server_16x16.png'></html>");
        auto dbIcon = new QLabel("<html><img src=':/robomongo/icons/database_16x16.png'></html>");
        auto collIcon = new QLabel("<html><img src=':/robomongo/icons/collection_16x16.png'></html>");

        selectedCollLay->addWidget(serverIcon,                      1, 0);
        selectedCollLay->addWidget(new QLabel("Server: "),          1, 1);
        selectedCollLay->addWidget(new QLabel("local_(2)"),         1, 2);
        selectedCollLay->addWidget(dbIcon,                          2, 0);
        selectedCollLay->addWidget(new QLabel("Database: "),        2, 1);
        selectedCollLay->addWidget(new QLabel(dbName),              2, 2);
        selectedCollLay->addWidget(collIcon,                        3, 0);
        selectedCollLay->addWidget(new QLabel("Collection: "),      3, 1);
        selectedCollLay->addWidget(new QLabel(collName),            3, 2);

        // Widgets related to Output 
        _formatComboBox = new QComboBox;
        _formatComboBox->addItem("JSON");
        _formatComboBox->addItem("CSV");
        VERIFY(connect(_formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_formatComboBox_change(int))));

        _fieldsLabel = new QLabel("Fields:");
        _fields = new QLineEdit; // todo: use textedit
        // Initially hidden
        _fieldsLabel->setHidden(true);
        _fields->setHidden(true);

        _query = new QLineEdit("{}"); // todo: JSON frame can be used
        _outputFileName = new QLineEdit;
        _outputDir = new QLineEdit;
        _browseButton = new QPushButton("...");
        _browseButton->setMaximumWidth(50);
        VERIFY(connect(_browseButton, SIGNAL(clicked()), this, SLOT(on_browseButton_clicked())));

        // Export summary widgets
        _exportOutput = new QTextEdit;
        QFontMetrics font(_exportOutput->font());
        _exportOutput->setFixedHeight((4+1.5) * (font.lineSpacing()));  // 4-line text edit
        _exportOutput->setReadOnly(true);

        _mongoExportOutput = new QTextEdit;
        _mongoExportOutput->setFixedHeight(4 * (font.lineSpacing() + 8));  // 4-line text edit
        _mongoExportOutput->setReadOnly(true);
        _mongoExportOutput->setHidden(true);

        // Attempt to fix issue for Windows High DPI button height is slightly taller than other widgets 
#ifdef Q_OS_WIN
        _browseButton->setMaximumHeight(HighDpiConstants::WIN_HIGH_DPI_BUTTON_HEIGHT);
#endif
        auto outputsInnerLay = new QGridLayout;
        outputsInnerLay->addWidget(new QLabel("Format:"),       0, 0);
        outputsInnerLay->addWidget(_formatComboBox,             0, 1, 1, 2);
        outputsInnerLay->addWidget(_fieldsLabel,                1, 0, Qt::AlignTop);
        outputsInnerLay->addWidget(_fields,                     1, 1, 1, 2);
        outputsInnerLay->addWidget(new QLabel("Query:"),        2, 0);
        outputsInnerLay->addWidget(_query,                      2, 1, 1, 2);
        outputsInnerLay->addWidget(new QLabel("File Name:"),    3, 0);
        outputsInnerLay->addWidget(_outputFileName,             3, 1, 1, 2);
        outputsInnerLay->addWidget(new QLabel("Directory:"),    4, 0);
        outputsInnerLay->addWidget(_outputDir,                  4, 1);
        outputsInnerLay->addWidget(_browseButton,               4, 2);
        //outputsInnerLay->addWidget(new QLabel("Result:"),       5, 0, Qt::AlignTop);
        //outputsInnerLay->addWidget(_exportOutput,           6, 0, 1, 3, Qt::AlignTop);

        auto manualLayout = new QGridLayout;
        auto cmdLabel = new QLabel("Command:");
        cmdLabel->setFixedHeight(cmdLabel->sizeHint().height());
        _manualExportCmd = new QTextEdit;
        QFontMetrics font1(_manualExportCmd->font());
        _manualExportCmd->setFixedHeight(2 * (font1.lineSpacing()+8));  // 2-line text edit
        manualLayout->addWidget(cmdLabel,                   0, 0, Qt::AlignTop);
        manualLayout->addWidget(_manualExportCmd,           1, 0, Qt::AlignTop);

        // Button box and Manual Mode button
        _modeButton = new QPushButton("Manual Mode");
        VERIFY(connect(_modeButton, SIGNAL(clicked()), this, SLOT(on_modeButton_clicked())));
        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        _buttonBox->button(QDialogButtonBox::Save)->setText("E&xport");
        _buttonBox->button(QDialogButtonBox::Save)->setMaximumWidth(70);
        _buttonBox->button(QDialogButtonBox::Cancel)->setMaximumWidth(70);
        VERIFY(connect(_buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        // Sub layouts
        auto serverIndicatorlayout = new QHBoxLayout();
        if (!serverName.isEmpty()) {
            serverIndicatorlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        }

        // Input layout
        _inputsGroupBox = new QGroupBox("Selected Collection");
        _inputsGroupBox->setLayout(selectedCollLay);
        _inputsGroupBox->setStyleSheet("QGroupBox::title { left: 0px }");
        _inputsGroupBox->setFixedHeight(_inputsGroupBox->sizeHint().height());

        // Outputs
        _autoOutputsGroup = new QGroupBox("Output Properties");
        _autoOutputsGroup->setLayout(outputsInnerLay);
        _autoOutputsGroup->setStyleSheet("QGroupBox::title { left: 0px }");
        _autoOutputsGroup->setFixedHeight(_autoOutputsGroup->sizeHint().height());

        // Manual Groupbox
        _manualGroupBox = new QGroupBox("Manual Export");
        _manualGroupBox->setLayout(manualLayout);
        _manualGroupBox->setStyleSheet("QGroupBox::title { left: 0px }");
        _manualGroupBox->setHidden(true);

        // Export Summary
        auto exportSummaryGroup = new QGroupBox("Export Summary");
        exportSummaryGroup->setStyleSheet("QGroupBox::title { left: 0px }");
        _viewOutputLink = new QLabel(SHOW_DETAILS);
        VERIFY(connect(_viewOutputLink, SIGNAL(linkActivated(QString)), this, SLOT(on_viewOutputLink(QString))));
        _viewOutputLink->setFixedHeight(_viewOutputLink->sizeHint().height());
        auto tempLayout = new QVBoxLayout();
        //tempLayout->setSizeConstraint(QLayout::SetFixedSize);
        tempLayout->addWidget(_exportOutput, Qt::AlignTop);
        tempLayout->addWidget(_viewOutputLink, Qt::AlignLeft | Qt::AlignTop);
        //tempLayout->addWidget(_mongoExportOutput);
        exportSummaryGroup->setLayout(tempLayout);
        exportSummaryGroup->setFixedHeight(exportSummaryGroup->sizeHint().height());

        // Buttonbox layout
        auto hButtonBoxlayout = new QHBoxLayout();
        hButtonBoxlayout->addStretch(1);
        hButtonBoxlayout->addWidget(_buttonBox);
        hButtonBoxlayout->addWidget(_modeButton);

        // Main Layout
        auto layout = new QVBoxLayout();
        layout->addWidget(_inputsGroupBox, Qt::AlignTop);
        //layout->addWidget(horline, Qt::AlignTop);
        layout->addWidget(_autoOutputsGroup, Qt::AlignTop);
        layout->addWidget(_manualGroupBox);
        layout->addWidget(exportSummaryGroup, Qt::AlignTop);
        layout->addLayout(hButtonBoxlayout);
        setLayout(layout);

        // todo: move to a function
        // Help user filling inputs automatically
        auto date = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        auto time = QDateTime::currentDateTime().toString("hh.mm.ss");
        auto timeStamp = date + "_" + time;
        auto format = _formatComboBox->currentIndex() == 0 ? "json" : "csv";

        _outputFileName->setText(dbName + "." + collName + "_" + timeStamp + "." + format);
        _outputDir->setText(defaultDir);
        _manualExportCmd->setText("mongoexport --db " + dbName + " --collection " + collName +
                                             " --out " + _outputDir->text() + _outputFileName->text());

        // todo: setExportArgs()
        // First set db and coll 
        _mongoExportArgs = " --db " + dbName + " --collection " + collName;

        _outputFileName->setFocus();
    }

    void ExportDialog::setOkButtonText(const QString &text)
    {
        _buttonBox->button(QDialogButtonBox::Save)->setText(text);
    }

    // todo: remove
    void ExportDialog::setInputLabelText(const QString &text)
    {
        //_inputLabel->setText(text);
    }

    void ExportDialog::accept()
    {
        QString mongoExport = "D:\\mongo_export\\bin\\mongoexport.exe";
        
        bool disable = false;
        enableDisableWidgets(disable);

        if (AUTO == _mode)
        {
            _exportOutput->clear();
            _exportOutput->setText("Exporting...");

            // If CSV append output format and fields
            if (_formatComboBox->currentIndex() == 1) {
                if (_fields->text().isEmpty()) {
                    QMessageBox::critical(this, "Error", "\"Fields\" option is required in CSV mode.");
                    return;
                }
                _mongoExportArgs.append(" --type=csv");
                _mongoExportArgs.append(" --fields " + _fields->text().replace(" ", ""));
            }

            if (!_query->text().isEmpty() && _query->text() != "{}") {
                _mongoExportArgs.append(" --query " + _query->text());
            }

            // Append file path and name
            auto absFilePath = _outputDir->text() + _outputFileName->text();
            _mongoExportArgs.append(" --out " + absFilePath);

            // Start mongoexport non-blocking
            _activeProcess->start(mongoExport + _mongoExportArgs);
        }
        else if (MANUAL == _mode)
        {
            _exportOutput->clear();
            _exportOutput->setText("Exporting...");

            // todo: check if _activeProcess->state() is QProcess::NotRunning
            // Start mongoexport non-blocking
            _activeProcess->start("D:\\mongo_export\\bin\\" + _manualExportCmd->toPlainText());
        }
    }

    void ExportDialog::ui_itemExpanded(QTreeWidgetItem *item)
    {
        auto categoryItem = dynamic_cast<ExplorerDatabaseCategoryTreeItem *>(item);
        if (categoryItem) {
            categoryItem->expand();
            return;
        }

        ExplorerServerTreeItem *serverItem = dynamic_cast<ExplorerServerTreeItem *>(item);
        if (serverItem) {
            serverItem->expand();
            return;
        }

        auto dirItem = dynamic_cast<ExplorerCollectionDirIndexesTreeItem *>(item);
        if (dirItem) {
            dirItem->expand();
        }
    }

    void ExportDialog::ui_itemDoubleClicked(QTreeWidgetItem *item, int column)
    {
        // todo
    }

    void ExportDialog::on_browseButton_clicked()
    {
        // Select output directory
        QString origDir = QFileDialog::getExistingDirectory(this, tr("Select Directory"), defaultDir,
                                             QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        auto dir = QDir::toNativeSeparators(origDir);

        QApplication::activeModalWidget()->raise();
        QApplication::activeModalWidget()->activateWindow();

        if (dir.isNull())
            return;

        _outputDir->setText(dir + "\\");
    }

    void ExportDialog::on_formatComboBox_change(int index)
    {
        bool const isCsv = static_cast<bool>(index);
        _fieldsLabel->setVisible(isCsv);
        _fields->setVisible(isCsv);
        
        // todo: divide ui_itemClicked()
        //ui_itemClicked(_treeWidget->currentItem());
    }

    void ExportDialog::on_modeButton_clicked()
    {
        _exportOutput->clear();
        _mode = (AUTO == _mode ? MANUAL : AUTO);
        _modeButton->setText(AUTO == _mode ? "Manual Mode" : "Auto Mode");
        _autoOutputsGroup->setVisible(AUTO == _mode);
        _manualGroupBox->setVisible(MANUAL == _mode);
        setMinimumSize(AUTO == _mode ? AUTO_MODE_SIZE : MANUAL_MODE_SIZE);
        _inputsGroupBox->setTitle(AUTO == _mode ? "Selected Collection" : "Selected Server");
        adjustSize();
    }

    void ExportDialog::on_exportFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        bool enable = true;
        enableDisableWidgets(enable);

        // Extract absolute file path
        QString absFilePath;
        if (AUTO == _mode) 
        {
            absFilePath = _outputDir->text() + _outputFileName->text();
        }
        else if (MANUAL == _mode)
        {
            // extract absolute file path string
            QStringList strlist1 = _manualExportCmd->toPlainText().split("--out");
            if (strlist1.size() > 1) {
                QString str1 = strlist1[1];
                QStringList strlist2 = str1.split("--");
                if (strlist2.size() > 1) {
                    absFilePath = strlist2[0];
                }
                else {
                    absFilePath = str1;
                }
            }
        }
        absFilePath.replace(" ", "");  // todo: handle paths with white spaces

        // todo: also check process exit code
        // Check exported file exists and mongoexport output does not contain error
        QFileInfo const file(absFilePath);
        _mongoExportOutputStr = _activeProcess->readAllStandardError(); // Extract mongoexport command output
        if (file.exists() && file.isFile() && _mongoExportOutputStr.contains("exported")) {
            QStringList splitA = _mongoExportOutputStr.split("exported");
            QStringList splitB = splitA[1].split("records");
            _exportResult = "Export Successful: \n" 
                            "Exported file: " + absFilePath + "\n"
                            "Number of records exported:" + splitB[0];
            _exportOutput->setText(_exportResult);
        }
        else {
            _exportOutput->setText("Export Failed.\n");
            _exportOutput->append("Output:\n" + _mongoExportOutputStr);
        }

        _exportOutput->moveCursor(QTextCursor::Start);
    }

    void ExportDialog::on_processErrorOccurred(QProcess::ProcessError error)
    {
        bool enable = true;
        enableDisableWidgets(enable);

        if (QProcess::FailedToStart == error) {
            _exportOutput->setText("Error: \"mongoexport\" process failed to start. Either the "
                "invoked program is missing, or you may have insufficient permissions to invoke the program.");
        }
        else if (QProcess::Crashed == error) {
            _exportOutput->setText("Error: \"mongoexport\" process crashed some time after starting"
                " successfully..");
        }
        else {
            _exportOutput->setText("Error: \"mongoexport\" process failed. Error code: "
                + QString::number(error));
        }

        _exportOutput->moveCursor(QTextCursor::Start);
    }

    void ExportDialog::on_viewOutputLink(QString)
    {
        QMessageBox::information(this, "Details", _mongoExportOutputStr);
    }
    
    Indicator *ExportDialog::createDatabaseIndicator(const QString &database)
    {
        return new Indicator(GuiRegistry::instance().databaseIcon(), database);
    }

    Indicator *ExportDialog::createCollectionIndicator(const QString &collection)
    {
        return new Indicator(GuiRegistry::instance().collectionIcon(), collection);
    }

    void ExportDialog::enableDisableWidgets(bool enable) const
    {
        // Auto mode widgets
        //_treeWidget->setEnabled(enable);
        _formatComboBox->setEnabled(enable);
        _fieldsLabel->setEnabled(enable);
        _fields->setEnabled(enable);
        _query->setEnabled(enable);
        _outputFileName->setEnabled(enable);
        _outputDir->setEnabled(enable);
        _browseButton->setEnabled(enable);
        _buttonBox->button(QDialogButtonBox::Save)->setEnabled(enable);
        _modeButton->setEnabled(enable);

        // Manual mode widgets
        //_treeWidget->setEnabled(enable);
        _manualExportCmd->setEnabled(enable);
        _buttonBox->button(QDialogButtonBox::Save)->setEnabled(enable);
        _modeButton->setEnabled(enable);
    }
}
