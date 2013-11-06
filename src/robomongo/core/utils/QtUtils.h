#pragma once
#include <QString>
#include <QModelIndex>
#include <QEvent>
#include "ErrorInfo.hpp"

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

        template<typename derived_t,typename value_t>
        class Event : public QEvent
        {
        public:
            typedef value_t value_type;
            enum { EventType = User+1 };

            Event(QObject *const sender, const value_t &initValue, ErrorInfo er)
                :QEvent((QEvent::Type)derived_t::EventType),_sender(sender), _value(initValue), _errorInfo(er){};
            
            QObject *const sender() const
            {
                return _sender;
            }

            void setValue(const value_t &val)
            {
                _value = val;
            }

            value_t value() const
            {
                return _value;
            }

            ErrorInfo errorInfo() const
            {
                return _errorInfo;
            }

        private:
            QObject *const _sender;
            value_t _value;
            ErrorInfo _errorInfo;
        };
    }
}
