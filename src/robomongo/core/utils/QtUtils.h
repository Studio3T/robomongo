#pragma once
#include <QString>

QT_BEGIN_NAMESPACE
class QThread;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Robomongo
{
    namespace QtUtils
    {
        template<typename T>
        QString toQString(const T &value);

        template<typename T>
        T toStdString(const QString &value);

        void cleanUpThread(QThread *const thread);

        void clearChildItems(QTreeWidgetItem *root);
    }
}
