#include "robomongo/gui/dialogs/CopyCollectionDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"


namespace Robomongo
{
    const QSize CopyCollection::minimumSize = QSize(300, 150);

    CopyCollection::CopyCollection(const QString &serverName, const QString &database,
                                               const QString &collection, QWidget *parent) :
        QDialog(parent),
        _currentServerName(serverName),
        _currentDatabase(database)
    {
        App::MongoServersContainerType servers = AppRegistry::instance().app()->getServers();
        QSet<QString> uniqueConnectionsNames;
        for (App::MongoServersContainerType::const_iterator it = servers.begin(); it != servers.end(); ++it) {
             MongoServer *server = *it;
             if (server->isConnected()) {
                 _servers.push_back(server);
                 uniqueConnectionsNames.insert(QtUtils::toQString(server->connectionRecord()->connectionName()));
             }
        }
        
        setWindowTitle("Copy Collection");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumSize(minimumSize);

        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), _currentServerName);

        QFrame *hline = new QFrame();
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        _buttonBox = new QDialogButtonBox(this);
        _buttonBox->setOrientation(Qt::Horizontal);
        _buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        _buttonBox->button(QDialogButtonBox::Save)->setText("Copy");
        VERIFY(connect(_buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(_buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->addStretch(1);
        hlayout->addWidget(_buttonBox);

        QHBoxLayout *vlayout = new QHBoxLayout();
        vlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        vlayout->addWidget(new Indicator(GuiRegistry::instance().databaseIcon(), _currentDatabase), 0, Qt::AlignLeft);
        vlayout->addWidget(new Indicator(GuiRegistry::instance().collectionIcon(), collection), 0, Qt::AlignLeft);

        QVBoxLayout *serverlayout = new QVBoxLayout();
        serverlayout->setContentsMargins(0, 3, 0, 0);
        _serverComboBox = new QComboBox();
        QLabel *serverLabel = new QLabel("Select server:");
        QLabel *description = new QLabel(
            QString("Copy <b>%1</b> collection to database on this or another server. "
                "You need to be already connected to destination server, in order to see this server in the list below. "
                "This operation will <i>not</i> overwrite existing documents with the same _id.")
                .arg(collection));

        description->setWordWrap(true);
        serverlayout->addWidget(description);
        serverlayout->addSpacing(14);
        serverlayout->addWidget(serverLabel);
        serverlayout->addWidget(_serverComboBox);

        QVBoxLayout *databaselayout = new QVBoxLayout();
        databaselayout->setContentsMargins(0, 8, 0, 7);
        _databaseComboBox = new QComboBox();
        QLabel *databaseLabel = new QLabel("Select database:");
        databaselayout->addWidget(databaseLabel);
        databaselayout->addWidget(_databaseComboBox);        
        VERIFY(connect(_serverComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDatabaseComboBox(int))));

        _serverComboBox->addItems(uniqueConnectionsNames.toList());
        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(vlayout);
        layout->addWidget(hline);
        layout->addLayout(serverlayout);
        layout->addLayout(databaselayout);
        layout->addLayout(hlayout);
        setLayout(layout);
    }

    void CopyCollection::updateDatabaseComboBox(int index)
    {
        _databaseComboBox->clear();
        const QString &curentServerName = _serverComboBox->currentText();
        MongoServer *server = NULL;
        for (App::MongoServersContainerType::const_iterator it = _servers.begin(); it != _servers.end(); ++it) {
            MongoServer *ser = *it;
            if (curentServerName == QtUtils::toQString(ser->connectionRecord()->connectionName())) {
                server = ser;
                break;
            }
        }  
        _databaseComboBox->addItems(server->getDatabasesNames());
        if (_currentServerName == QtUtils::toQString(server->connectionRecord()->getFullAddress())) {
            _databaseComboBox->removeItem(_databaseComboBox->findText(_currentDatabase));
        }
    }

    MongoDatabase *CopyCollection::selectedDatabase()
    {
        MongoDatabase *result = NULL;
        const QString &serverName = _serverComboBox->currentText();
        const QString &dataBaseName = _databaseComboBox->currentText();
        if (!serverName.isEmpty() && !dataBaseName.isEmpty()) {
            MongoServer *server = _servers[_serverComboBox->currentIndex()];
            result = server->findDatabaseByName(QtUtils::toStdString(dataBaseName));
        }
        return result;
    }

    void CopyCollection::accept()
    {
        if (!selectedDatabase())
            return;

        QDialog::accept();
    }
}
