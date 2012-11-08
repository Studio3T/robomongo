#include "SettingsManager.h"
#include <QDir>
#include <QDebug>
#include "AppRegistry.h"

using namespace Robomongo;

SettingsManager::SettingsManager()
{
    QString home2 = QDir::homePath();

    qDebug() << home2;

    QString z = home2;
}

void SettingsManager::addConnection(const ConnectionRecord &connection)
{

}
