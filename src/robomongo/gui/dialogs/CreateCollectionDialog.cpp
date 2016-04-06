#include "robomongo/gui/dialogs/CreateCollectionDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>

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
		setWindowTitle("Create Collection v3");
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
		layout->addLayout(hlayout);
		setLayout(layout);

		_inputEdit->setFocus();
	}

	QString CreateCollectionDialog::databaseName() const
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

	void CreateCollectionDialog::accept()
	{
		if (_inputEdit->text().isEmpty())
			return;

		QDialog::accept();
	}

	Indicator *CreateCollectionDialog::createDatabaseIndicator(const QString &database)
	{
		return new Indicator(GuiRegistry::instance().databaseIcon(), database);
	}

	Indicator *CreateCollectionDialog::createCollectionIndicator(const QString &collection)
	{
		return new Indicator(GuiRegistry::instance().collectionIcon(), collection);
	}
}
