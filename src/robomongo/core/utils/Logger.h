#pragma once


// #define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kStorage

#include <QObject>
#include <QString>
#include <string>
#include <mongo/logger/log_severity.h>
#include "robomongo/core/utils/SingletonPattern.hpp"

namespace Robomongo
{  
    class Logger : public QObject, public Patterns::LazySingleton<Logger>
    {
        Q_OBJECT
        friend class Patterns::LazySingleton<Logger>;

    public:
        void print(const char *mess, mongo::logger::LogSeverity level, bool notify);
        void print(const std::string &mess, mongo::logger::LogSeverity level, bool notify);
        void print(const QString &mess, mongo::logger::LogSeverity level, bool notify);

    Q_SIGNALS:
        void printed(const QString &mess, mongo::logger::LogSeverity level);

    private:
        Logger();
        ~Logger();
    };

    template<typename T>
    inline void LOG_MSG(const T &mess, mongo::logger::LogSeverity level, bool notify = true)
    {
        return Logger::instance().print(mess, level, notify);
    }
}
