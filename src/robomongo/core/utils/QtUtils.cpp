#include "robomongo/core/utils/QtUtils.h"

#include <QTextCodec>
#include <QThread>
#include <QTreeWidgetItem>

namespace Robomongo
{
    namespace QtUtils
    {
        template<>
        QString toQString<std::string>(const std::string &value)
        {
            //static QTextCodec *LOCALECODEC = QTextCodec::codecForLocale();
            return QString::fromUtf8(value.c_str(), value.size());
        }

        template<>
        QString toQString<std::wstring>(const std::wstring &value)
        {
            return  QString((const QChar*)value.c_str(), value.length());
        }

        template<>
        std::string toStdString<std::string>(const QString &value)
        {
            QByteArray sUtf8 = value.toUtf8();
            return std::string(sUtf8.constData(), sUtf8.length());
        }

        template<>
        std::wstring toStdString<std::wstring>(const QString &value)
        {
            return std::wstring((wchar_t*)value.unicode(), value.length());
        }

        template<>
        std::string toStdStringSafe<std::string>(const QString &value)
        {
#ifdef Q_OS_WIN
            QByteArray latin = value.toLocal8Bit();
            return std::string(latin.constData(), latin.length());
#else
            QByteArray sUtf8 = value.toUtf8();
#endif    
        }

        void cleanUpThread(QThread *const thread)
        {
            if (thread&&thread->isRunning()) {
                //thread->stop();
                thread->wait();
            }
        }

        void clearChildItems(QTreeWidgetItem *const root)
        {
            int itemCount = root->childCount();
            for (int i = 0; i < itemCount; ++i) {
                QTreeWidgetItem *item = root->child(0);
                root->removeChild(item);
                delete item;
            }
        }
    }
}
