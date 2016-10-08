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
    //const QSize ExportDialog::dialogSize = QSize(500, 250);     // todo: remove

    namespace
    {
        const QString defaultDir = "D:\\exports\\";     // Default location

        auto const AUTO_MODE_SIZE = QSize(500, 550);
        auto const MANUAL_MODE_SIZE = QSize(500, 650);

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

    ExportDialog::ExportDialog(QWidget *parent) :
        QDialog(parent), _mode(SMART_MODE), _mongoExportArgs()
    {
        setWindowTitle("Export Collection");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        //setFixedSize(dialogSize);
        setMinimumSize(AUTO_MODE_SIZE);

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

        // 
        auto selectCollLabel = new QLabel("Select Collection To Export:");
        

        // Tree Widget
        _treeWidget = new QTreeWidget;
        _treeWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        _treeWidget->setIndentation(15);
        _treeWidget->setHeaderHidden(true);
        //_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        VERIFY(connect(_treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(ui_itemExpanded(QTreeWidgetItem *))));
       
        VERIFY(connect(_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), 
                       this, SLOT(ui_itemDoubleClicked(QTreeWidgetItem *, int))));

        // todo: use diffirent signal
        VERIFY(connect(_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
                       this, SLOT(ui_itemClicked(QTreeWidgetItem *))));
        
        //
        auto const& serversVec = AppRegistry::instance().app()->getServers();
        if (!serversVec.empty()){
            auto explorerServerTreeItem = new ExplorerServerTreeItem(_treeWidget, *serversVec.begin()); // todo
            _treeWidget->addTopLevelItem(explorerServerTreeItem);
            _treeWidget->setCurrentItem(explorerServerTreeItem);
            _treeWidget->setFocus();
        }

        // Widgets related to Output 
        _formatComboBox = new QComboBox;
        _formatComboBox->addItem("JSON");
        _formatComboBox->addItem("CSV");
        VERIFY(connect(_formatComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_formatComboBox_change(int))));

        _fieldsLabel = new QLabel("Fields:");
        _fields = new QLineEdit; // todo: make textedit
        // Initially hidden
        _fieldsLabel->setHidden(true);
        _fields->setHidden(true);

        _query = new QLineEdit; // todo: can use JSON frame

        auto outputFileLabel = new QLabel("File Name:");
        _outputFileName = new QLineEdit;

        auto outputDirLabel = new QLabel("Directory:");
        _outputDir = new QLineEdit;
        auto browseButton = new QPushButton("...");
        browseButton->setMaximumWidth(50);
        VERIFY(connect(browseButton, SIGNAL(clicked()), this, SLOT(on_browseButton_clicked())));

        // Attempt to fix issue for Windows High DPI button height is slightly taller than other widgets 
#ifdef Q_OS_WIN
        browseButton->setMaximumHeight(HighDpiContants::WIN_HIGH_DPI_BUTTON_HEIGHT);
#endif
        auto outputsInnerLay = new QGridLayout;
        outputsInnerLay->addWidget(new QLabel("Format"),    0, 0);
        outputsInnerLay->addWidget(_formatComboBox,         0, 1, 1, 2);
        outputsInnerLay->addWidget(_fieldsLabel,            1, 0, Qt::AlignTop);
        outputsInnerLay->addWidget(_fields,                 1, 1, 1, 2);
        outputsInnerLay->addWidget(new QLabel("Query"),     2, 0);
        outputsInnerLay->addWidget(_query,                  2, 1, 1, 2);
        outputsInnerLay->addWidget(outputFileLabel,         3, 0);
        outputsInnerLay->addWidget(_outputFileName,         3, 1, 1, 2);
        outputsInnerLay->addWidget(outputDirLabel,          4, 0);
        outputsInnerLay->addWidget(_outputDir,              4, 1);
        outputsInnerLay->addWidget(browseButton,            4, 2);

        auto manualLayout = new QGridLayout;
        
        auto cmdLabel = new QLabel("Command:");
        cmdLabel->setFixedHeight(cmdLabel->sizeHint().height());
        _manualExportCmd = new QTextEdit;
        QFontMetrics font(_manualExportCmd->font());
        _manualExportCmd->setFixedHeight(2 * (font.lineSpacing()+8));  // 2-line text edit
        auto resultLabel = new QLabel("Result:");
        resultLabel->setFixedHeight(cmdLabel->sizeHint().height());
        _manualExportOutput = new QTextEdit;
        _manualExportOutput->setFixedHeight(6 * (font.lineSpacing()+8));  // 6-line text edit
        _manualExportOutput->setReadOnly(true);
        
        manualLayout->addWidget(cmdLabel,                   0, 0, Qt::AlignTop);
        manualLayout->addWidget(_manualExportCmd,           1, 0, Qt::AlignTop);
        manualLayout->addWidget(resultLabel,                2, 0, Qt::AlignTop);
        manualLayout->addWidget(_manualExportOutput,        3, 0, Qt::AlignTop);

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
        auto inputsGroupBox = new QGroupBox("Input Properties");
        auto inputsLay = new QVBoxLayout;
        //inputsLay->addLayout(dbNameLay);
        //inputsLay->addLayout(collNameLay);
        inputsLay->addWidget(selectCollLabel);
        inputsLay->addWidget(_treeWidget);
        inputsGroupBox->setLayout(inputsLay);
        inputsGroupBox->setStyleSheet("QGroupBox::title { left: 0px }");
        //inputsGroupBox->setFlat(true);

        // Outputs
        _outputsGroupBox = new QGroupBox("Output Properties");
        _outputsGroupBox->setLayout(outputsInnerLay);
        _outputsGroupBox->setStyleSheet("QGroupBox::title { left: 0px }");

        // Manual Groupbox
        _manualGroupBox = new QGroupBox("Manual Export");
        _manualGroupBox->setLayout(manualLayout);
        _manualGroupBox->setStyleSheet("QGroupBox::title { left: 0px }");
        _manualGroupBox->setHidden(true);

        // Buttonbox layout
        auto hButtonBoxlayout = new QHBoxLayout();
        hButtonBoxlayout->addStretch(1);
        hButtonBoxlayout->addWidget(_buttonBox);
        hButtonBoxlayout->addWidget(_modeButton);

        // Main Layout
        auto layout = new QVBoxLayout();
        //layout->addLayout(serverIndicatorlayout);
        //layout->addWidget(horline);
        layout->addWidget(inputsGroupBox);
        //layout->addWidget(_treeWidget);
        layout->addWidget(horline);
        layout->addWidget(_outputsGroupBox);
        layout->addWidget(_manualGroupBox);
        layout->addLayout(hButtonBoxlayout);
        setLayout(layout);

        _treeWidget->setFocus();
    }

    // todo: remove
    //QString ExportDialog::databaseName() const
    //{
    //    //return _inputEdit->text();
    //}

    void ExportDialog::setOkButtonText(const QString &text)
    {
        _buttonBox->button(QDialogButtonBox::Save)->setText(text);
    }

    // todo: remove
    void ExportDialog::setInputLabelText(const QString &text)
    {
        //_inputLabel->setText(text);
    }

    // todo: remove
    void ExportDialog::setInputText(const QString &text)
    {
        //_inputEdit->setText(text);
        //_inputEdit->selectAll();
    }

    void ExportDialog::accept()
    {
        QString mongoExport = "D:\\mongo_export\\bin\\mongoexport.exe";
        
        if (SMART_MODE == _mode)
        {
            // todo: setExportArgs()
            // First set db and coll 
            _mongoExportArgs = " --db " + _dbName + " --collection " + _collName;

            // If CSV append output format and fields
            if (_formatComboBox->currentIndex() == 1) {
                if (_fields->text().isEmpty()) {
                    QMessageBox::critical(this, "Error", "\"Fields\" option is required in CSV mode.");
                    return;
                }
                _mongoExportArgs.append(" --type=csv");
                _mongoExportArgs.append(" --fields " + _fields->text().replace(" ", ""));
            }

            if (!_query->text().isEmpty()) {
                _mongoExportArgs.append(" --query " + _query->text());
            }

            // Append file path and name
            auto absFilePath = _outputDir->text() + _outputFileName->text();
            _mongoExportArgs.append(" --out " + absFilePath);

            // Run mongoexport process
            QProcess process(this);
            process.start(mongoExport + _mongoExportArgs);
            process.setProcessChannelMode(QProcess::MergedChannels);
            auto const error = process.error();
            process.waitForFinished();
            if (QProcess::FailedToStart == error) {
                // todo: make error str const string
                QMessageBox::critical(this, "Export Failed", "Error: \"mongoexport\" failed to start. "
                    "Either the invoked program is missing, or you may have insufficient permissions to "
                    "invoke the program.");
            }
            // todo: else if crashed
            else {  // process has run and finished normally
                absFilePath.replace(" ", "");  // todo: handle white spaces inside the path as well
                QFileInfo const file(absFilePath);
                // Extract mongoexport command output
                QString output = process.readAllStandardError();
                // check if file exists and if yes: Is it really a file and no directory?
                if (file.exists() && file.isFile() && !output.contains("error")) {
                    QMessageBox::information(this, "Export Successful", "Exported File: \n" + absFilePath + "\n\n" +
                        "Output:\n" + output );
                }
                else {
                    QMessageBox::critical(this, "Export Failed", "Output:\n" + output);
                }
            }
        }
        else if (MANUAL_MODE == _mode)
        {
            _manualExportOutput->clear();
            QProcess process(this);
            process.start("D:\\mongo_export\\bin\\" + _manualExportCmd->toPlainText());
            process.setProcessChannelMode(QProcess::MergedChannels);
            auto const error = process.error();
            process.waitForFinished();
            if (QProcess::FailedToStart == error) {
                _manualExportOutput->setText("Error: \"mongoexport\" failed to start. Either the "
                    "invoked program is missing, or you may have insufficient permissions to invoke the program.");
            }
            // todo: else if crashed
            else {  // process has run and finished normally
                // extract absolute file path string
                QString absFilePath;
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
                absFilePath.replace(" ", "");  // todo: handle path with white spaces
                QFileInfo const file(absFilePath);
                // Extract mongoexport command output
                QString output = process.readAllStandardError();
                // check if file exists and if yes: Is it really a file and no directory?
                if (file.exists() && file.isFile() && !output.contains("error")) {
                    _manualExportOutput->setText("Export Successful: \n""Exported file: " + absFilePath + "\n");
                    _manualExportOutput->append("Output:\n" + output);
                }
                else {
                    _manualExportOutput->setText("Export Failed.\n");
                    _manualExportOutput->append("Output:\n" + output);
                }
            }
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
        //auto collectionItem = dynamic_cast<ExplorerCollectionTreeItem*>(item);
        //if (collectionItem) {
        //    AppRegistry::instance().app()->openShell(collectionItem->collection());
        //    return;
        //}

        // todo
        //auto dbTreeItem = dynamic_cast<ExplorerDatabaseTreeItem*>(item);
        //if (dbTreeItem) {
        //    dbTreeItem->applySettingsForExportDialog();
        //}
    }

    void ExportDialog::ui_itemClicked(QTreeWidgetItem *current)
    {
        auto collectionItem = dynamic_cast<ExplorerCollectionTreeItem*>(current);
        if (!collectionItem)
            return;

        _dbName = QString::fromStdString(collectionItem->collection()->database()->name());
        _collName = QString::fromStdString(collectionItem->collection()->name());

        auto date = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        auto time = QDateTime::currentDateTime().toString("hh.mm.ss");
        auto timeStamp = date + "_" + time;
        auto format = _formatComboBox->currentIndex() == 0 ? "json" : "csv";

        _outputFileName->setText(_dbName + "." + _collName + "_" + timeStamp + "." + format);
        _outputDir->setText(defaultDir);
        _manualExportCmd->setText("mongoexport --db " + _dbName + " --collection " + _collName +
                                             " --out " + _outputDir->text() + _outputFileName->text());
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
        ui_itemClicked(_treeWidget->currentItem());
    }

    void ExportDialog::on_modeButton_clicked()
    {
        _mode = (SMART_MODE == _mode ? MANUAL_MODE : SMART_MODE);
        _modeButton->setText(SMART_MODE == _mode ? "Manual Mode" : "Auto Mode");
        _outputsGroupBox->setVisible(SMART_MODE == _mode);
        _manualGroupBox->setVisible(MANUAL_MODE == _mode);
        setMinimumSize(SMART_MODE == _mode ? AUTO_MODE_SIZE : MANUAL_MODE_SIZE);
        adjustSize();
    }

    Indicator *ExportDialog::createDatabaseIndicator(const QString &database)
    {
        return new Indicator(GuiRegistry::instance().databaseIcon(), database);
    }

    Indicator *ExportDialog::createCollectionIndicator(const QString &collection)
    {
        return new Indicator(GuiRegistry::instance().collectionIcon(), collection);
    }

}
