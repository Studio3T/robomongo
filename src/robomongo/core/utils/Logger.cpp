#include "robomongo/core/utils/Logger.h"

#include <QDir>
#include <QMetaType>

#include "robomongo/core/utils/QtUtils.h"

namespace
{
    std::string getLoggerPath()
    {
        static std::string path = 
            Robomongo::QtUtils::toStdString(QString("%1/" PROJECT_NAME_LOWERCASE ".log").arg(QDir::tempPath()));

        return path;
    }
}

namespace Robomongo
{
    Logger::Logger()
    {
        // v0.9
        // qRegisterMetaType<mongo::logger::LogSeverity>("mongo::LogLevel");
        std::string path = getLoggerPath();
        QFile file(QtUtils::toQString(path)); //delete file if it size more than 5mb
        if (file.exists() && file.size() > 5 * 1024 * 1024) {
            file.remove();
        }
        // v0.9
        // mongo::initLogging(path,true);
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

    void Logger::print(const QString &mess, mongo::logger::LogSeverity level, bool notify)
    {
        // v0.9
//        LOG(level) << "[" PROJECT_NAME_TITLE "] " << QtUtils::toStdString(mess) << std::endl;
        if (notify) {
            // Make uniform log level strings e.g "Error: ", "Info: " etc...
            auto logLevelStr = QString::fromStdString(level.toStringData().toString());
            if (!logLevelStr.isEmpty()) {
                logLevelStr = logLevelStr.toLower();
                logLevelStr[0] = logLevelStr[0].toUpper();
                logLevelStr += ": ";
            }
            emit printed(logLevelStr + mess, level);
        }
    }
}
