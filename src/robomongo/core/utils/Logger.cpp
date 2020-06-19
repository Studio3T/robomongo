#include "robomongo/core/utils/Logger.h"

#include <QDir>
#include <QMetaType>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/App.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    void sendLog(
        QObject *sender, LogEvent::LogLevel const& severity,
        std::string const& msg, bool const informUser /*= false*/)
    {
        AppRegistry::instance().bus()->send(
            AppRegistry::instance().app(),
            new LogEvent(sender, msg, severity, informUser)
        );
    }

    Logger::Logger()
    {
        QFile file {
            QString("%1/" PROJECT_NAME_LOWERCASE ".log").arg(QDir::tempPath()) 
        }; 
        //delete file if it size more than 5mb
        if (file.exists() && file.size() > 5 * 1024 * 1024)
            file.remove();      
    }

    Logger::~Logger()
    {   
    }

    void Logger::print(const char *mess, mongo::logger::LogSeverity level, bool notify)
    {
        print(std::string(mess), level, notify);
    }

    void Logger::print(const std::string &mess, mongo::logger::LogSeverity level, bool notify)
    {       
        print(QtUtils::toQString(mess), level, notify);
    }

    void Logger::print(const QString &msg, mongo::logger::LogSeverity level, bool notify)
    {
        if (!notify)
            return;

        // Make uniform log level strings e.g "Error: ", "Info: " etc...
        auto logLevelStr = QString::fromStdString(level.toStringData().toString());
        if (!logLevelStr.isEmpty()) {
            logLevelStr = logLevelStr.toLower();
            logLevelStr[0] = logLevelStr[0].toUpper();
            logLevelStr += ": ";
        }
        emit printed(logLevelStr + msg.simplified(), level);
    }
}
