#include "robomongo/utils/common.h"

#include <QFileInfo>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/utils/Logger.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/utils/string_operations.h"

namespace Robomongo
{
    void genericEventErrorHandler(Event *event, const std::string &userFriendlyMessage, EventBus* bus, 
                                  QObject* sender)
    {
        if (!event->isError())
            return;

        std::string errMsg = captilizeFirstChar(event->error().errorMessage());
        LOG_MSG(userFriendlyMessage + " " + errMsg, mongo::logger::LogSeverity::Error());

        if (bus && sender)
            bus->publish(new OperationFailedEvent(sender, errMsg, userFriendlyMessage));
        else
            LOG_MSG("Failed to publish OperationFailedEvent.", mongo::logger::LogSeverity::Error());
    }

    bool fileExists(const QString &filePath)
    {
        QFileInfo fileInfo(filePath);
        return fileInfo.exists() && fileInfo.isFile();
    }

}   // end of name space Robomongo
