#pragma once
#include <QString>

QT_BEGIN_NAMESPACE
class QThread;
class QTreeWidgetItem;
QT_END_NAMESPACE

#ifdef QT_NO_DEBUG
#define VERIFY(x) (x)
#else //QT_NO_DEBUG
#define VERIFY(x) Q_ASSERT(x)
#endif //QT_NO_DEBUG

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
