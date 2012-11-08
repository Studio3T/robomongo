#include <QApplication>
#include <QSettings>
#include <QList>
#include "mainwindow.h"

#include "qjson/parser.h"

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

    a.setOrganizationDomain("robomongo.com");
    a.setOrganizationName("Robomongo");
    a.setApplicationName("Robomongo");

    QList<Login> logins;

    logins.append(Login("One", "1"));
    logins.append(Login("Two", "2"));
    logins.append(Login("Three", "3"));


    QJson::Parser parser;

    bool ok;

    // json is a QString containing the data to convert
    QVariantMap result = parser.parse (" { \"tesst2\" : \"value\" } ", &ok).toMap();



    MainWindow w;
    w.show();
    
    return a.exec();
}
