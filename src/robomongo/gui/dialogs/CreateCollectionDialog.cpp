#include "robomongo/gui/dialogs/CreateCollectionDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QCheckBox>

#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    const QSize CreateCollectionDialog::dialogSize = QSize(300, 150);

    CreateCollectionDialog::CreateCollectionDialog(const QString &serverName, const QString &database,
        const QString &collection, QWidget *parent) :
        QDialog(parent)
    {
        setWindowTitle("Create Collection"); 
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        //setFixedSize(dialogSize);
        setMinimumWidth(300);

        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);

        QFrame *hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        _inputEdit = new QLineEdit();
        _inputLabel = new QLabel("Database Name:");
        _inputEdit->setMaxLength(maxLenghtName);

        _tabWidget = new QTabWidget(this);
        _tabWidget->addTab(createOptionsTab(), tr("Options"));
        _tabWidget->addTab(createStorageEngineTab(), tr("Storage Engine"));
        //_tabWidget->addTab(createTextSearchTab(), tr("Validator"));
        _tabWidget->setTabsClosable(false);

        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        _buttonBox->button(QDialogButtonBox::Save)->setText("C&reate");
        VERIFY(connect(_buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->addStretch(1);
        hlayout->addWidget(_buttonBox);

        QHBoxLayout *vlayout = new QHBoxLayout();
        if (!serverName.isEmpty())
            vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        if (!database.isEmpty())
            vlayout->addWidget(createDatabaseIndicator(database), 0, Qt::AlignLeft);
        if (!collection.isEmpty())
            vlayout->addWidget(createCollectionIndicator(collection), 0, Qt::AlignLeft);

        QVBoxLayout *namelayout = new QVBoxLayout();
        namelayout->setContentsMargins(0, 7, 0, 7);
        namelayout->addWidget(_inputLabel);
        namelayout->addWidget(_inputEdit);
        
        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(vlayout);
        layout->addWidget(hline);
        layout->addLayout(namelayout);
        layout->addWidget(_tabWidget);
        layout->addLayout(hlayout);
        setLayout(layout);

        _inputEdit->setFocus();
    }

    QString CreateCollectionDialog::getCollectionName() const
    {
        return _inputEdit->text();
    }

    void CreateCollectionDialog::setOkButtonText(const QString &text)
    {
        _buttonBox->button(QDialogButtonBox::Save)->setText(text);
    }

    void CreateCollectionDialog::setInputLabelText(const QString &text)
    {
        _inputLabel->setText(text);
    }

    void CreateCollectionDialog::setInputText(const QString &text)
    {
        _inputEdit->setText(text);
        _inputEdit->selectAll();
    }

    bool CreateCollectionDialog::isCapped() const
    {
        return (_cappedCheckBox->checkState() == Qt::Checked);
    }

    long long CreateCollectionDialog::getSizeInputEditValue() const
    {
        return (_sizeInputEdit->text().toLongLong());
    }
    
    int CreateCollectionDialog::getMaxDocNumberInputEditValue() const
    {
        return (_maxDocNumberInputEdit->text().toInt());
    }

    bool CreateCollectionDialog::isCheckedAutoIndexid() const
    {
        return (_autoIndexCheckBox->checkState() == Qt::Checked);
    }

    bool CreateCollectionDialog::isCheckedUsePowerOfTwo() const
    {
        return (_usePowerOfTwoSizeCheckBox->checkState() == Qt::Checked);
    }

    bool CreateCollectionDialog::isCheckedNoPadding() const
    {
        return (_noPaddingCheckBox->checkState() == Qt::Checked);
    }

    void CreateCollectionDialog::accept()
    {
        if (_inputEdit->text().isEmpty())
            return;

        QDialog::accept();
    }

    void CreateCollectionDialog::cappedCheckBoxStateChanged(int newState)
    {
        _sizeInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
        _maxDocNumberInputEdit->setEnabled((Qt::Checked == static_cast<Qt::CheckState>(newState)));
    }

    Indicator *CreateCollectionDialog::createDatabaseIndicator(const QString &database)
    {
        return new Indicator(GuiRegistry::instance().databaseIcon(), database);
    }

    Indicator *CreateCollectionDialog::createCollectionIndicator(const QString &collection)
    {
        return new Indicator(GuiRegistry::instance().collectionIcon(), collection);
    }

    QWidget* CreateCollectionDialog::createOptionsTab()
    {
        QWidget *options = new QWidget(this);

        _cappedCheckBox = new QCheckBox(tr("Create capped collection"), options);
        _sizeInputLabel = new QLabel(tr("Maximum size in bytes: "));
        _sizeInputLabel->setContentsMargins(22, 0, 0, 0);
        _sizeInputEdit = new QLineEdit();
        _sizeInputEdit->setEnabled(false);
        _maxDocNumberInputLabel = new QLabel(tr("Maximum number of documents: "));
        _maxDocNumberInputLabel->setContentsMargins(22, 0, 0, 0);
        _maxDocNumberInputEdit = new QLineEdit();
        _maxDocNumberInputEdit->setEnabled(false);
        _autoIndexCheckBox = new QCheckBox(tr("Auto index _id"), options);
        _autoIndexCheckBox->setChecked(true);
        _usePowerOfTwoSizeCheckBox = new QCheckBox(tr("Use power-of-2 sizes"), options);
        _noPaddingCheckBox = new QCheckBox(tr("No Padding"), options);

        VERIFY(connect(_cappedCheckBox, SIGNAL(stateChanged(int)), this, SLOT(cappedCheckBoxStateChanged(int))));

        QGridLayout *layout = new QGridLayout;
        layout->addWidget(_cappedCheckBox,              0, 0, 1, 2);
        layout->addWidget(_sizeInputLabel,              1, 0);
        layout->addWidget(_sizeInputEdit,               1, 1);
        layout->addWidget(_maxDocNumberInputLabel,      2, 0);
        layout->addWidget(_maxDocNumberInputEdit,       2, 1);
        layout->addWidget(_autoIndexCheckBox,           3, 0);
        layout->addWidget(_usePowerOfTwoSizeCheckBox,   4, 0);
        layout->addWidget(_noPaddingCheckBox,           5, 0);
        layout->setAlignment(Qt::AlignTop);
        options->setLayout(layout);

        return options;
    }

    QWidget* CreateCollectionDialog::createStorageEngineTab()
    {
        QWidget *textSearch = new QWidget(this);
        // todo...
        return textSearch;
    }
}
