#include <QApplication>
#include <QSettings>
#include <QList>
#include <QMessageBox>
#include <QDir>
#include "mainwindow.h"

#include "qjson/parser.h"
#include "settings/SettingsManager.h"

using namespace Robomongo;

struct Login {
     QString userName;
     QString password;

     Login(const QString & userName, const QString & password) : userName(userName), password(password)
     {

     }
};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationDomain("robomongo.codm");
    a.setOrganizationName("Robomongo");
    a.setApplicationName("Robomongo");

    QList<Login> logins;

    logins.append(Login("One", "1"));
    logins.append(Login("Two", "2"));
    logins.append(Login("Three", "3"));


    QJson::Parser parser;

    bool ok;

    // json is a QString containing the data to convert
    QVariantMap result = parser.parse (" { \"tedsfdddddddddst2\" : \"value\" } ", &ok).toMap();

    SettingsManager * manager = new SettingsManager();

    QMessageBox msg;
    msg.setText(QDir::homePath());
    msg.show();

    MainWindow w;
    w.show();
    
    return a.exec();
}
