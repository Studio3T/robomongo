#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QHash>
#include <QMessageBox>
#include "ConnectionsDialog.h"
#include "Dialogs/EditConnectionDialog.h"

using namespace Robomongo;
/*
** Constructs Dialog
*/
ConnectionsDialog::ConnectionsDialog(SettingsManager * settingsManager) : QDialog()
{
    _settingsManager = settingsManager;
    connect(_settingsManager, SIGNAL(connectionAdded(ConnectionRecord)), this, SLOT(add(ConnectionRecord)));
    connect(_settingsManager, SIGNAL(connectionUpdated(ConnectionRecord)), this, SLOT(update(ConnectionRecord)));
    connect(_settingsManager, SIGNAL(connectionRemoved(ConnectionRecord)), this, SLOT(remove(ConnectionRecord)));


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
	
    foreach(ConnectionRecord connectionModel, _settingsManager->connections())
        add(connectionModel);

    _listWidget->setViewMode(QListView::ListMode);
}

void ConnectionsDialog::set(QListWidgetItem *item, ConnectionRecord connection)
{
    item->setText(connection.connectionName());
    item->setData(Qt::UserRole, QVariant::fromValue<ConnectionRecord>(connection));
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
    EditConnectionDialog editDialog(connectionModel);

	// Do nothing if not accepted
	if (editDialog.exec() != QDialog::Accepted)
		return;

    _settingsManager->updateConnection(connectionModel);


//	_viewModel->updateConnection(connectionModel);
}

void ConnectionsDialog::add()
{
    ConnectionRecord newModel;
	EditConnectionDialog editDialog(newModel);

	// Do nothing if not accepted
	if (editDialog.exec() != QDialog::Accepted)
		return;

    _settingsManager->addConnection(newModel);
    _listWidget->setFocus();

//    _viewModel->insertConnection(newModel);*/
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

    // Getting stored pointer
    QVariant data = currentItem->data(Qt::UserRole);
    ConnectionRecord connectionModel = data.value<ConnectionRecord>();
    _settingsManager->removeConnection(connectionModel);
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

    QMessageBox box;
    box.setText(QString("Number of items: %1").arg(_settingsManager->connections().count()));
    box.exec();

    return;

	hide();
//	_viewModel->selectConnection(connection);
    QDialog::accept();
}

void ConnectionsDialog::add(ConnectionRecord connection)
{
    QListWidgetItem * item = new QListWidgetItem;
    set(item, connection);
    _listWidget->addItem(item);
    _hash.insert(connection, item);
}

void ConnectionsDialog::update(ConnectionRecord connection)
{
    QListWidgetItem *item = _hash.value(connection);
    if (!item)
        return;

    set(item, connection);
}

void ConnectionsDialog::remove(ConnectionRecord connection)
{
    QListWidgetItem *item = _hash.value(connection);
    if (!item)
        return;

    delete item;
}
