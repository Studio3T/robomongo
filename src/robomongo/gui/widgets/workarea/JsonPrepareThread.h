#pragma once

#include <QThread>
#include <vector>

#include "robomongo/core/Core.h"

#include "robomongo/core/Enums.h"

namespace Robomongo
{
    /*
    ** In this thread we are running task to prepare JSON string from list of BSON objects
    */
    class JsonPrepareThread : public QThread
    {
        Q_OBJECT

    public:
        /*
        ** Constructor
        */
        JsonPrepareThread(const std::vector<MongoDocumentPtr> &bsonObjects, UUIDEncoding uuidEncoding, SupportedTimes timeZone);
        void stop();
   Q_SIGNALS:
        /**
         * @brief Signals when all parts prepared
         */
        void done();

        /**
         * @brief Signals when json part is ready
         */
        void partReady(const QString &part);

    protected:

        /*
        ** Overload function
        */
        virtual void run();
    private:
        /*
        ** List of documents
        */
        const std::vector<MongoDocumentPtr> _bsonObjects;
        const UUIDEncoding _uuidEncoding;
        const SupportedTimes _timeZone;
        volatile bool _stop;
    };
}
