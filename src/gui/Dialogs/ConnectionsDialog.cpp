#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>
#include "ConnectionsDialog.h"

using namespace Robomongo;
/*
** Constructs Dialog
*/
ConnectionsDialog::ConnectionsDialog(QList<ConnectionRecord> & records) : QDialog()
{
    _connections = records;
	_listWidget = new QListWidget;
    connect(_listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)), this, SLOT(accept()));

	QPushButton * addButton = new QPushButton("Add");
	connect(addButton, SIGNAL(clicked()), this, SLOT(add()));

	QPushButton * editButton = new QPushButton("Edit");
	connect(editButton, SIGNAL(clicked()), this, SLOT(edit()));

	QPushButton * removeButton = new QPushButton("Remove");
	connect(removeButton, SIGNAL(clicked()), this, SLOT(remove()));

	QPushButton * cancelButton = new QPushButton("Cancel");
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

	QPushButton * connectButton = new QPushButton("Connect");
    connectButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowRight));
	connect(connectButton, SIGNAL(clicked()), this, SLOT(accept()));

	QHBoxLayout * bottomLayout = new QHBoxLayout;
	bottomLayout->addWidget(connectButton, 1, Qt::AlignRight);
	bottomLayout->addWidget(cancelButton);

	QVBoxLayout * firstColumnLayout = new QVBoxLayout;
	firstColumnLayout->addWidget(_listWidget);
	firstColumnLayout->addLayout(bottomLayout);
	//firstColumnLayout->addWidget(connectButton, 5, Qt::AlignRight);

	QVBoxLayout * secondColumnLayout = new QVBoxLayout;
	secondColumnLayout->setAlignment(Qt::AlignTop);
	secondColumnLayout->addWidget(addButton);
	secondColumnLayout->addWidget(editButton);
	secondColumnLayout->addWidget(removeButton);

	QHBoxLayout * mainLayout = new QHBoxLayout(this);
	mainLayout->addLayout(firstColumnLayout);
	mainLayout->addLayout(secondColumnLayout);

	setWindowTitle("Connections...");
    //setWindowIcon(AppRegistry::instance().serverIcon());

    // Remove help button (?)
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    refresh();
}

void ConnectionsDialog::refresh()
{
	_listWidget->clear();
	
    foreach(ConnectionRecord connectionModel, _connections)
	{
		QListWidgetItem * item = new QListWidgetItem;
        item->setText(connectionModel.connectionName());

		// storing pointer to our connection record
        item->setData(Qt::UserRole, QVariant::fromValue<ConnectionRecord>(connectionModel));

		_listWidget->addItem(item);
	}

	_listWidget->setViewMode(QListView::ListMode);
}

void ConnectionsDialog::edit()
{
	QListWidgetItem * currentItem = _listWidget->currentItem();

	// Do nothing if no item selected
	if (currentItem == 0)
		return;

	// Getting stored pointer
	QVariant data = currentItem->data(Qt::UserRole);
    ConnectionRecord connectionModel = data.value<ConnectionRecord>();

    int i = 90;
/*	EditConnectionDialog editDialog(connectionModel);

	// Do nothing if not accepted
	if (editDialog.exec() != QDialog::Accepted)
		return;
*/
//	_viewModel->updateConnection(connectionModel);
}

void ConnectionsDialog::add()
{
    /*
	ConnectionDialogViewModel * newModel = new ConnectionDialogViewModel(_viewModel.get());

	EditConnectionDialog editDialog(newModel);

	// Do nothing if not accepted
	if (editDialog.exec() != QDialog::Accepted)
		return;

    _viewModel->insertConnection(newModel);*/
}

void ConnectionsDialog::remove()
{
	QListWidgetItem * currentItem = _listWidget->currentItem();

	// Do nothing if no item selected
	if (currentItem == 0)
		return;

    // Ask user
    int answer = QMessageBox::question(this,
            "Connections",
            "Really delete selected connection?",
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer != QMessageBox::Yes)
        return;

	// Getting stored pointer
	QVariant data = currentItem->data(Qt::UserRole);
//	ConnectionDialogViewModel  * connection = reinterpret_cast<ConnectionDialogViewModel*>(data.value<void*>());

//	_viewModel->deleteConnection(connection);
}

void ConnectionsDialog::accept()
{
	QListWidgetItem * currentItem = _listWidget->currentItem();

	// Do nothing if no item selected
	if (currentItem == 0)
		return;

	// Getting stored pointer
	QVariant data = currentItem->data(Qt::UserRole);
//	ConnectionDialogViewModel * connection = reinterpret_cast<ConnectionDialogViewModel*>(data.value<void*>());

	hide();
//	_viewModel->selectConnection(connection);
	QDialog::accept();
}
