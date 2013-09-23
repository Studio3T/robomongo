#pragma once

#include <QObject>
#include <QString>
#include <string>
#include <mongo/util/log.h>
#include "robomongo/core/utils/SingletonPattern.hpp"

namespace Robomongo
{  
    class Logger
        : public QObject, public Patterns::LazySingleton<Logger>
    {
        friend class Patterns::LazySingleton<Logger>;
        Q_OBJECT
    public:
        void print(const char *mess, mongo::LogLevel level, bool notify);
        void print(const std::string &mess, mongo::LogLevel level, bool notify);
        void print(const QString &mess, mongo::LogLevel level, bool notify);        
    Q_SIGNALS:
        void printed(const QString &mess, mongo::LogLevel level);

    private:
        Logger();
        ~Logger();
    };

    template<typename T>
    inline void LOG_MSG(const T &mess, mongo::LogLevel level, bool notify = true)
    {
        return Logger::instance().print(mess, level, notify);
    }
}
