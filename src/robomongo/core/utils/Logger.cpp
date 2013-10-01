#include "robomongo/core/utils/Logger.h"

#include <QDir>
#include <QMetaType>
#include "robomongo/core/utils/QtUtils.h"

namespace
{
    std::string getLoggerPath()
    {
        static std::string path = Robomongo::QtUtils::toStdString(QString("%1/"PROJECT_NAME_LOWERCASE".log").arg(QDir::tempPath()));
        return path;
    }
}

namespace Robomongo
{
    Logger::Logger()
    {
        qRegisterMetaType<mongo::LogLevel>("mongo::LogLevel");
        std::string path = getLoggerPath();
        QFile file(QtUtils::toQString(path)); //delete file if it size more than 5mb
        if (file.exists() && file.size() > 5 * 1024 * 1024) {
            file.remove();
        }
        mongo::initLogging(path,true);
    }

    Logger::~Logger()
    {   
    }

    void Logger::print(const char *mess, mongo::LogLevel level, bool notify)
    {
        print(std::string(mess), level, notify);
    }

    void Logger::print(const std::string &mess, mongo::LogLevel level, bool notify)
    {       
        print(QtUtils::toQString(mess), level, notify);
    }

    void Logger::print(const QString &mess, mongo::LogLevel level, bool notify)
    {        
        LOG(level) << "["PROJECT_NAME_TITLE"] " << QtUtils::toStdString(mess) << std::endl;
        if (notify)
            emit printed(mess, level);
    }
}
