#pragma once

#include <QObject>
#include <QString>
#include <string>

#include <mongo/logger/log_severity.h>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/utils/SingletonPattern.hpp"

namespace Robomongo
{  
    class Logger : public QObject, public Patterns::LazySingleton<Logger>
    {
        Q_OBJECT
        friend class Patterns::LazySingleton<Logger>;

    public:
        void print(const char *msg, mongo::logger::LogSeverity level, bool notify);
        void print(const std::string &msg, mongo::logger::LogSeverity level, bool notify);
        void print(const QString &msg, mongo::logger::LogSeverity level, bool notify);

    Q_SIGNALS:
        void printed(const QString &msg, mongo::logger::LogSeverity level);

    private:
        Logger();
        ~Logger();
    };

    // Use in main thread
    template<typename T>
    inline void LOG_MSG(const T &msg, mongo::logger::LogSeverity level, bool notify = true)
    {
        return Logger::instance().print(msg, level, notify);
    }
    
    // Use in worker threads (e.g. MongoWorker) to send LogEvent to main thread (App class) 
    void sendLog(
        QObject *sender, LogEvent::LogLevel const& severity,
        std::string const& msg, bool const informUser = false);
}