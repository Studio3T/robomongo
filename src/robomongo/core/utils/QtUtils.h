#pragma once
#include <QString>
#include <QModelIndex>

QT_BEGIN_NAMESPACE
class QThread;
class QTreeWidgetItem;
class QAbstractItemModel;
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

        std::string toStdString(const QString &value);

        std::string toStdStringSafe(const QString &value);

        void cleanUpThread(QThread *const thread);

        void clearChildItems(QTreeWidgetItem *root);

        template<typename Type>
        inline Type item(const QModelIndex &index)
        {
            return static_cast<Type>(index.internalPointer());
        }

        struct HackQModelIndex
        {
            int r, c;
            void* i;
            const QAbstractItemModel *m;
        };
    }
}
