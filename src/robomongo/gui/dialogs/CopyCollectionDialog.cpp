#include "robomongo/gui/dialogs/CopyCollectionDialog.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"


namespace Robomongo
{
    CopyCollection::CopyCollection(const QString &serverName, const QString &database,
                                               const QString &collection, QWidget *parent) :
        QDialog(parent),
        _servers( AppRegistry::instance().app()->getServers())
    {
        setWindowTitle("Create Database");
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint); // Remove help button (?)
        setMinimumWidth(300);

        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), serverName);

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
        vlayout->addWidget(new Indicator(GuiRegistry::instance().databaseIcon(), database), 0, Qt::AlignLeft);
        vlayout->addWidget(new Indicator(GuiRegistry::instance().collectionIcon(), collection), 0, Qt::AlignLeft);

        QVBoxLayout *serverlayout = new QVBoxLayout();
        _serverComboBox = new QComboBox();
        QLabel *serverLabel = new QLabel("Select server");
        serverlayout->addWidget(serverLabel);
        serverlayout->addWidget(_serverComboBox);

        QVBoxLayout *databaselayout = new QVBoxLayout();
        _databaseComboBox = new QComboBox();
        QLabel *databaseLabel = new QLabel("Select database");
        databaselayout->addWidget(databaseLabel);
        databaselayout->addWidget(_databaseComboBox);        
        VERIFY(connect(_serverComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDatabaseComboBox(int))));

        for (App::MongoServersContainerType::const_iterator it = _servers.begin(); it != _servers.end(); ++it)
        {
            MongoServer *server = *it;
            _serverComboBox->addItem(QtUtils::toQString(server->connectionRecord()->connectionName()));
        }        

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
        MongoServer *server = _servers[index];
        _databaseComboBox->addItems(server->getDatabasesNames());
    }

    MongoDatabase *CopyCollection::selectedDatabase()
    {
        MongoDatabase *result = NULL;
        const QString &serverName = _serverComboBox->currentText();
        const QString &dataBaseName = _databaseComboBox->currentText();
        if (!serverName.isEmpty()&&!dataBaseName.isEmpty()){
            MongoServer *server = _servers[_serverComboBox->currentIndex()];
            result = server->findDatabaseByName(QtUtils::toStdString<std::string>(dataBaseName));
        }
        return result;
    }

    void CopyCollection::accept()
    {
        if(!selectedDatabase())
            return;

        QDialog::accept();
    }
}
