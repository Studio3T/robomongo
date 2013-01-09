#include <QApplication>
#include <QMessageBox>
#include <QList>
#include <QSharedPointer>
#include <QProcess>
#include <mongo/client/dbclient.h>

#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/Core.h"
#include "robomongo/gui/dialogs/ConnectionsDialog.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Add ./plugins directory to library path
    QString plugins = QString("%1%2plugins")
            .arg(QApplication::applicationDirPath())
            .arg(QDir::separator());

    QApplication::addLibraryPath(plugins);

    //qRegisterMetaType<ConnectionSettingsPtr>("ConnectionSettingsPtr");

//    QProcessEnvironment proc = QProcessEnvironment::systemEnvironment();
//    QString str = proc.value("LD_LIBRARY_PATH");

//    qDebug() << str;

    AppRegistry::instance().settingsManager()->save();

    QRect screenGeometry = QApplication::desktop()->availableGeometry();
    QSize size(screenGeometry.width() - 450, screenGeometry.height() - 165);

    MainWindow win;
    win.resize(size);

    int x = (screenGeometry.width() - win.width()) / 2;
    int y = (screenGeometry.height() - win.height()) / 2;
    win.move(x, y);
    win.show();

    QTreeView *tree = new QTreeView();
    QListView *list = new QListView();
    QTableView *table = new QTableView();

    QSplitter splitter;
    splitter.addWidget( tree );
    splitter.addWidget( list );
    splitter.addWidget( table );

    QStandardItemModel model( 5, 2 );
    for( int r=0; r<5; r++ )
      for( int c=0; c<2; c++)
      {
        QStandardItem *item = new QStandardItem( QString("Row:%0, Column:%1").arg(r).arg(c) );

        if( c == 0 )
          for( int i=0; i<3; i++ )
          {
            QStandardItem *child = new QStandardItem( QString("Item %0").arg(i) );
            child->setEditable( false );
            item->appendRow( child );
          }

        model.setItem(r, c, item);
      }

    model.setHorizontalHeaderItem( 0, new QStandardItem( "Foo" ) );
    model.setHorizontalHeaderItem( 1, new QStandardItem( "Bar-Baz" ) );

    tree->setModel( &model );
    list->setModel( &model );
    table->setModel( &model );

    list->setSelectionModel( tree->selectionModel() );

    table->setSelectionModel( tree->selectionModel() );

    table->setSelectionBehavior( QAbstractItemView::SelectRows );
    table->setSelectionMode( QAbstractItemView::SingleSelection );

    //splitter.show();


    return app.exec();
}
