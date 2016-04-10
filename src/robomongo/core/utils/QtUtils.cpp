#include "robomongo/core/utils/QtUtils.h"

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

        std::string toStdString(const QString &value)
        {
            QByteArray sUtf8 = value.toUtf8();
            return std::string(sUtf8.constData(), sUtf8.length());
        }

        std::string toStdStringSafe(const QString &value)
        {
#ifdef Q_OS_WIN
            QByteArray sUtf8 = value.toLocal8Bit();            
#else
            QByteArray sUtf8 = value.toUtf8();
#endif    
            return std::string(sUtf8.constData(), sUtf8.length());
        }

        void cleanUpThread(QThread *const thread)
        {
            if (thread && thread->isRunning()) {
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
