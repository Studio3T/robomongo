#include "robomongo/gui/dialogs/ExportDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
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
    const QSize ExportDialog::dialogSize = QSize(500, 200);     // todo: remove

    namespace
    {
        const QString defaultDir = "D:\\exports\\";     // Default location

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
        QDialog(parent), _mongoExportArgs()
    {
        setWindowTitle("Export Collection");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        //setFixedSize(dialogSize);
        setMinimumSize(350, 250);

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
        auto _treeWidget = new QTreeWidget;
        _treeWidget->setContextMenuPolicy(Qt::DefaultContextMenu);
        _treeWidget->setIndentation(15);
        _treeWidget->setHeaderHidden(true);
        //_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        VERIFY(connect(_treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem *)), this, SLOT(ui_itemExpanded(QTreeWidgetItem *))));
       
        VERIFY(connect(_treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), 
                       this, SLOT(ui_itemDoubleClicked(QTreeWidgetItem *, int))));

        // todo: use diffirent signal
        VERIFY(connect(_treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
                       this, SLOT(ui_itemClicked(QTreeWidgetItem *, QTreeWidgetItem *))));
        
        //
        auto const& serversVec = AppRegistry::instance().app()->getServers();
        if (!serversVec.empty()){
            auto explorerServerTreeItem = new ExplorerServerTreeItem(_treeWidget, *serversVec.begin()); // todo
            _treeWidget->addTopLevelItem(explorerServerTreeItem);
            _treeWidget->setCurrentItem(explorerServerTreeItem);
            _treeWidget->setFocus();
        }

        // Widgets related to Output 
        auto typeLabel = new QLabel("Format");
        _formatComboBox = new QComboBox;
        _formatComboBox->addItem("JSON");
        _formatComboBox->addItem("CSV");

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
        outputsInnerLay->addWidget(typeLabel,               0, 0);
        outputsInnerLay->addWidget(_formatComboBox,         0, 1, 1, 2);
        outputsInnerLay->addWidget(outputFileLabel,         1, 0);
        outputsInnerLay->addWidget(_outputFileName,         1, 1, 1, 2);
        outputsInnerLay->addWidget(outputDirLabel,          2, 0);
        outputsInnerLay->addWidget(_outputDir,              2, 1);
        outputsInnerLay->addWidget(browseButton,            2, 2);

        // Button box
        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        _buttonBox->button(QDialogButtonBox::Save)->setText("E&xport");
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

        // Output layout
        auto outputsGroupBox = new QGroupBox("Output Properties");
        outputsGroupBox->setLayout(outputsInnerLay);
        outputsGroupBox->setStyleSheet("QGroupBox::title { left: 0px }");

        // Buttonbox layout
        auto hButtonBoxlayout = new QHBoxLayout();
        hButtonBoxlayout->addStretch(1);
        hButtonBoxlayout->addWidget(_buttonBox);


        // Main Layout
        auto layout = new QVBoxLayout();
        //layout->addLayout(serverIndicatorlayout);
        //layout->addWidget(horline);
        layout->addWidget(inputsGroupBox);
        //layout->addWidget(_treeWidget);
        layout->addWidget(horline);
        layout->addWidget(outputsGroupBox);
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
        
        // Append file path and name
        auto absFilePath = _outputDir->text() + _outputFileName->text();
        _mongoExportArgs.append(" --out " + absFilePath);

        // result: If process cannot be started -2 is returned. If process crashes, -1 is returned. 
        // If successful returns 0, otherwise, the process' exit code is returned.
        auto const result = QProcess::execute(mongoExport + _mongoExportArgs);

        if (0 == result) {
            QMessageBox::information(this, "Export Successful", "Ouput File: \n" + absFilePath);
        }
        else {
            // todo: more error details
            QMessageBox::critical(this, "Export Failed", "Error Code: " + QString::number(result));
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

    void ExportDialog::ui_itemClicked(QTreeWidgetItem *current, QTreeWidgetItem *previous)
    {
        auto collectionItem = dynamic_cast<ExplorerCollectionTreeItem*>(current);
        if (!collectionItem)
            return;

        auto dbName = QString::fromStdString(collectionItem->collection()->database()->name());
        auto collName = QString::fromStdString(collectionItem->collection()->name());
        auto date = QDateTime::currentDateTime().toString("dd.MM.yyyy");
        auto time = QDateTime::currentDateTime().toString("hh.mm.ss");
        auto timeStamp = date + "_" + time;
        auto format = _formatComboBox->currentIndex() == 0 ? "json" : "csv";

        _outputFileName->setText(dbName + "." + collName + "_" + timeStamp + "." + format);
        _outputDir->setText(defaultDir);
        
        // todo: setExportArgs()
        // For now set only db and coll 
        _mongoExportArgs = " --db " + dbName + " --collection " + collName;
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

    Indicator *ExportDialog::createDatabaseIndicator(const QString &database)
    {
        return new Indicator(GuiRegistry::instance().databaseIcon(), database);
    }

    Indicator *ExportDialog::createCollectionIndicator(const QString &collection)
    {
        return new Indicator(GuiRegistry::instance().collectionIcon(), collection);
    }

}
