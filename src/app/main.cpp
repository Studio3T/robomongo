#include <QApplication>
#include <QSettings>
#include <QList>
#include "mainwindow.h"

struct Login {
     QString userName;
     QString password;

     Login(const QString & userName, const QString & password) : userName(userName), password(password)
     {

     }
};

bool removeArrayItem(QSettings & settings, const QString & strArrayName, int 	iIndex )
{
   bool fRet = false;

   settings.beginReadArray(strArrayName);
   const QStringList keys(settings.allKeys());
   QStringList values;

   if (iIndex < keys.size())
   {
      for (int i = 0; i < keys.size(); i++)
         values.insert(i, settings.value(keys.at(i)).toString());
      settings.endArray();

      settings.beginGroup(strArrayName);
      settings.remove("");

      const QChar cIndex2Remove(iIndex + 1 + 48);
      QChar cIndex2Write;
      for (int i = 0; i < keys.size(); i++)
      {
         const QString& strKey = keys.at(i);
         const QChar cIndex2Read(strKey.at(0));
         if (cIndex2Read.isDigit())
         {
            if (cIndex2Read != cIndex2Remove)
            {
               cIndex2Write = cIndex2Read > cIndex2Remove ? QChar(cIndex2Read.digitValue() - 1 + 48) : QChar(cIndex2Read.digitValue() + 48);
               settings.setValue(cIndex2Write + strKey.mid(1), values.at(i));
            }
         }
      }
      settings.setValue("size", QString(cIndex2Write));
      settings.endGroup();
      fRet = true;
   }

   return(fRet);
}

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

    QSettings settings;
    settings.beginWriteArray("logins");
    for (int i = 0; i < logins.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("userName", logins.at(i).userName);
        settings.setValue("password", logins.at(i).password);
    }
    settings.endArray();

    // remove array
    int size = settings.beginReadArray("logins");
    for (int i = 0; i < size; ++i) {
        removeArrayItem(settings, "logins", i);
    }

    logins.removeAt(1);
    settings.beginWriteArray("logins");
    for (int i = 0; i < logins.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("userName", logins.at(i).userName);
        settings.setValue("password", logins.at(i).password);
    }
    settings.endArray();

//    settings.remove("logins/1",);

    MainWindow w;
    w.show();
    
    return a.exec();
}
