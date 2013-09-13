#include "robomongo/core/utils/Logger.h"

#include <QDir>
#include "robomongo/core/utils/QtUtils.h"
#include "mongo/util/log.h"

namespace
{
    mongo::LabeledLevel pc(PROJECT_NAME, mongo::LL_DEBUG );
    std::string getLoggerPath()
    {
        static std::string path = Robomongo::QtUtils::toStdString<std::string>(QString("%1/."PROJECT_NAME"log.txt").arg(QDir::homePath()));
        return path;
    }
}

namespace Robomongo
{
    Logger::Logger()
    {
        mongo::initLogging(getLoggerPath(),true);
    }

    Logger::~Logger()
    {
    }

    void Logger::print(const char *mess)
    {
        print(std::string(mess));
    }

    void Logger::print(const std::string &mess)
    {       
        print(QtUtils::toQString(mess));
    }

    void Logger::print(const QString &mess)
    {
        LOG(pc) << QtUtils::toStdString<std::string>(mess) << std::endl;
        emit printed(mess);
    }
}