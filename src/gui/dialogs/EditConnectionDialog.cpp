#include <QtGui>
#include "EditConnectionDialog.h"
#include "AppRegistry.h"
#include <QList>

using namespace Robomongo;

/**
 * @brief Constructs dialog with specified connection
 */
EditConnectionDialog::EditConnectionDialog(ConnectionSettings *connection) : QDialog(),
    _connection(connection)
{
    //setContentsMargins(7, 7, 7, 7);
    setWindowTitle("Edit connection");
    // Remove help button (?)
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    _tabWidget = new QTabWidget;

    QPushButton *saveButton = new QPushButton("&Save");
    saveButton->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowRight));
    saveButton->setDefault(true);
    connect(saveButton, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *cancelButton = new QPushButton("&Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    QPushButton *testButton = new QPushButton("&Test");
    testButton->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
    connect(testButton, SIGNAL(clicked()), this, SLOT(testConnection()));

    _connectionName = new QLineEdit(_connection->connectionName());
    _serverAddress = new QLineEdit(_connection->databaseAddress());
    _serverPort = new QLineEdit(QString::number(_connection->databasePort()));
    _defaultDatabaseName = new QLineEdit();

    _userName = new QLineEdit();
    _userNameLabel = new QLabel("User Name");
    _userPassword = new QLineEdit();
    _databaseName = new QLineEdit();

    _serverPort->setFixedWidth(80);

    _credentialsTree = new QTreeWidget;
    _credentialsTree->setColumnCount(3);

    QStringList colums;
    colums << "Database" << "User Name" << "Password";
    _credentialsTree->setHeaderLabels(colums);
    _credentialsTree->header()->setResizeMode(0, QHeaderView::Stretch);
    _credentialsTree->header()->setResizeMode(1, QHeaderView::Stretch);
    _credentialsTree->header()->setResizeMode(2, QHeaderView::Stretch);

    _credentialsView = new QTableView();
    _credentialsModel = new CredentialModel(_connection->credentials());
//    _credentialsModel->setHorizontalHeaderItem(0, new QStandardItem("Database"));
//    _credentialsModel->setHorizontalHeaderItem(1, new QStandardItem("User Name"));
//    _credentialsModel->setHorizontalHeaderItem(2, new QStandardItem("Password"));
//    _credentialsModel->setVerticalHeaderItem(0, new QStandardItem("*"));

    _credentialsView->setModel(_credentialsModel);

    _credentialsView->setSelectionBehavior( QAbstractItemView::SelectRows );
    _credentialsView->setSelectionMode( QAbstractItemView::SingleSelection );

    _credentialsView->installEventFilter(this);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->addWidget(testButton, 1, Qt::AlignLeft);
    bottomLayout->addWidget(saveButton, 1, Qt::AlignRight);
    bottomLayout->addWidget(cancelButton);

    QGridLayout *editLayout = new QGridLayout;
    editLayout->addWidget(new QLabel("Connection Name"),      0, 0);
    editLayout->addWidget(_connectionName,         0, 1);
//    editLayout->addWidget(new QLabel("Server Host"),    1, 0);
//    editLayout->addWidget(_serverAddress,          1, 1);
//    editLayout->addWidget(new QLabel("Server Port"),      2, 0);
//    editLayout->addWidget(_serverPort,             2, 1, Qt::AlignLeft);
//    editLayout->addWidget(new QLabel("Default Database"), 3, 0);
//    editLayout->addWidget(_defaultDatabaseName,    3, 1, Qt::AlignLeft);

    _useAuth = new QCheckBox("Authenticate");
    _useAuth->setStyleSheet("margin-top: 13px");
    connect(_useAuth, SIGNAL(toggled(bool)), this, SLOT(authChecked(bool)));

    _auth = new AuthWidget();
    ServerWidget *ser = new ServerWidget();

    _tabWidget->addTab(ser, "Connection");
    _tabWidget->addTab(_auth, "Authentication");
    _tabWidget->addTab(new QLabel("advanced"), "Advanced");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
//    mainLayout->addLayout(editLayout);
//    mainLayout->addSpacing(10);
    mainLayout->addWidget(_tabWidget);
//    mainLayout->addLayout(editLayout);
//    mainLayout->addWidget(_useAuth);
//    mainLayout->addWidget(_auth);
//    mainLayout->addSpacing(10);
//    mainLayout->addWidget(new QLabel("Authenticate:"));
//    mainLayout->addWidget(_credentialsView);
//    mainLayout->addSpacing(10);
    mainLayout->addLayout(bottomLayout);
    //mainLayout->setSizeConstraint(QLayout::SetFixedSize);

    updateCredentialTree();
    authChecked(false);
}

/**
 * @brief Accept() is called when user agree with entered data.
 */
void EditConnectionDialog::accept()
{
    _connection->setConnectionName(_connectionName->text());
    _connection->setDatabaseAddress(_serverAddress->text());
    _connection->setDatabasePort(_serverPort->text().toInt());
//    _connection->setUserName(_userName->text());
//    _connection->setUserPassword(_userPassword->text());
//    _connection->setDatabaseName(_databaseName->text());

    QDialog::accept();
}

void EditConnectionDialog::addCredential(CredentialSettings *credential)
{
/*    QList<QStandardItem *> items;
    items << new QStandardItem("aga");
    items << new QStandardItem("aga2");
    items << new QStandardItem("aga3");

    _credentialsModel->appendRow(items);*/

//            _credentialsModel->appendRow();

/*    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setText(0, credential->databaseName());
    item->setText(1, credential->userName());
    item->setText(2, credential->userPassword());
    _credentialsTree->setEditTriggers(QAbstractItemView::CurrentChanged);
    _credentialsTree->addTopLevelItem(item);
    */
}

/**
 * @brief Close event handler
 */
void EditConnectionDialog::closeEvent(QCloseEvent *event)
{
    if (canBeClosed())
        event->accept();
    else
        event->ignore();
}

/**
 * @brief Check that it is okay to close this window
 *        (there is no modification of data, that we possibly can loose)
 */
bool EditConnectionDialog::canBeClosed()
{
    bool unchanged =
        _connection->connectionName() == _connectionName->text()
        && _connection->databaseAddress() == _serverAddress->text()
        && QString::number(_connection->databasePort()) == _serverPort->text()
        /*&& _connection->userName() == _userName->text()
        && _connection->userPassword() == _userPassword->text()
        && _connection->databaseName() == _databaseName->text()*/;

    // If data was unchanged - simply close dialog
    if (unchanged)
        return true;

    // Ask user
    int answer = QMessageBox::question(this,
            "Connections",
            "Close this window and loose you changes?",
            QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);

    if (answer == QMessageBox::Yes)
        return true;

    return false;
}

/**
 * @brief Key press event handler
 */
void EditConnectionDialog::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
        close();
    else if (e->key() == Qt::Key_Delete)
        deleteCredential();
    else
        QDialog::keyPressEvent(e);
}

/**
 * @brief Test current connection
 */
void EditConnectionDialog::testConnection()
{
    bool res = true; //_connection->test(_serverAddress->text(), _serverPort->text(), _userName->text(), _userPassword->text());

    if (res)
        QMessageBox::information(NULL, "Success!", "Success! Connection exists.", "Ok");
}

void EditConnectionDialog::deleteCredential()
{
    QModelIndexList list = _credentialsView->selectionModel()->selectedRows();

    for (int i = 0; i < list.size(); i++) {
          QModelIndex index = list[i];

          if (index.row() < _connection->credentials().count()) {
              _credentialsModel->remove(index.row());
              _credentialsModel->updateAll();
          }
    }
}

void EditConnectionDialog::authChecked(bool checked)
{
    _auth->setVisible(checked);
    _auth->_databaseName->setFocus();
//    _userName->setVisible(checked);
//    _userNameLabel->setVisible(checked);
//    _userPassword->setVisible(checked);
//    _databaseName->setVisible(checked);

//    layout()->update();
}

void EditConnectionDialog::updateCredentialTree()
{
    addCredential(new CredentialSettings("d", "", ""));
}


int CredentialModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _credentials.count() + 1;
}

int CredentialModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant CredentialModel::data(const QModelIndex &index, int role) const
{
    if ((role == Qt::EditRole || role == Qt::DisplayRole) && index.row() == _credentials.count())
         return QVariant ();

    if (index.column() == 2 && role == Qt::DisplayRole) {
            return "******";
    }

    if (index.isValid() && role == Qt::DisplayRole || role == Qt::EditRole) {
        CredentialSettings *credential = _credentials.value(index.row());
        switch(index.column()) {
            case 0 : return credential->databaseName();
            case 1 : return credential->userName();
            case 2 : return credential->userPassword();
        }
    }


    return QVariant();
}

QVariant CredentialModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
            case 0: return "Database";
            case 1: return "User Name";
            case 2: return "Password";
        }

        return QVariant();
    }

    if (orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        if (section == _credentials.count())
            return "*";
        else
            return QString::number(section);
    }

    return QVariant();
}

bool CredentialModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {

        CredentialSettings *credential;
        if (index.row() == _credentials.count()) {
            if  (!value.toString().isEmpty()) {
                credential = new CredentialSettings(value.toString(), "", "");
                _credentials.append(credential);
            } else
                return false;
        } else {
            credential = _credentials.value(index.row());
        }
        switch(index.column()) {
            case 0 : credential->setDatabaseName(value.toString()); break;
            case 1 : credential->setUserName(value.toString()); break;
            case 2 : credential->setUserPassword(value.toString()); break;
        }

        emit dataChanged(createIndex(0, 0), createIndex(_credentials.count() + 10, 2));
        emit headerDataChanged(Qt::Vertical, 0, _credentials.count() + 10);
        return true;
    }

    return false;
}

Qt::ItemFlags CredentialModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void CredentialModel::updateAll()
{
    emit dataChanged(createIndex(0, 0), createIndex(_credentials.count() + 10, 2));
    emit headerDataChanged(Qt::Vertical, 0, _credentials.count() + 10);
}

void CredentialModel::remove(int at)
{
    _credentials.removeAt(at);
}


AuthWidget::AuthWidget()
{
    QLabel *authLabel = new QLabel(
        "<nobr>The <b>admin</b> database is unique in MongoDB.</nobr> Users with normal access "
        "to the <b>admin</b> database have read and write access to <i>all "
        "databases</i>.");

    authLabel->setWordWrap(true);

    _userName = new QLineEdit();
    _userNameLabel = new QLabel("User Name");
    _userPassword = new QLineEdit();
    _databaseName = new QLineEdit();

    QGridLayout *_authLayout = new QGridLayout;
    _authLayout->addWidget(authLabel,  0, 0, 1, 2);
    _authLayout->addWidget(new QLabel("Database"),  1, 0);
    _authLayout->addWidget(_databaseName,           1, 1);
    _authLayout->addWidget(_userNameLabel,          2, 0);
    _authLayout->addWidget(_userName,               2, 1);
    _authLayout->addWidget(new QLabel("Password"),  3, 0);
    _authLayout->addWidget(_userPassword,           3, 1);
//    _authLayout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(_authLayout);
}


ServerWidget::ServerWidget()
{
    QLabel *authLabel = new QLabel(
        "Choose any connection name that will help you to identify this connection.");
    authLabel->setWordWrap(true);
    authLabel->setContentsMargins(0, -2, 0, 20);

    QLabel *serverLabel = new QLabel(
        "Specify host and port of MongoDB server. Host can be either IP or domain name.");
    serverLabel->setWordWrap(true);

    _connectionName = new QLineEdit("localhost");
    _serverAddress = new QLineEdit("localhost");
    _serverPort = new QLineEdit(QString::number(27017));
    _serverPort->setFixedWidth(60);

    _defaultDatabaseName = new QLineEdit();

    QGridLayout *connectionLayout = new QGridLayout;
    connectionLayout->addWidget(new QLabel("Name:"),      1, 0);
    connectionLayout->addWidget(_connectionName,         1, 1, 1, 3);
    connectionLayout->addWidget(authLabel,               2, 1, 1, 3);

    //QGridLayout *serverLayout = new QGridLayout;
    connectionLayout->addWidget(serverLabel, 4, 1, 1, 3);
    connectionLayout->addWidget(new QLabel("Address:"),    3, 0);
    connectionLayout->addWidget(_serverAddress,          3, 1);
    connectionLayout->addWidget(new QLabel(":"),      3, 2);
    connectionLayout->addWidget(_serverPort,             3, 3);
//    serverLayout->setSizeConstraint(QLayout::SetMaximumSize);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(connectionLayout);
//    mainLayout->addSpacing(13);
//    mainLayout->addLayout(serverLayout);


//    editLayout->addWidget(new QLabel("Default Database"), 3, 0);
//    editLayout->addWidget(_defaultDatabaseName,    3, 1, Qt::AlignLeft);

    setLayout(mainLayout);


}
