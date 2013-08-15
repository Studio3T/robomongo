#pragma once

#include <QWidget>
#include <QThread>
QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/domain/Enums.h"

namespace Robomongo
{
    class BsonTreeWidget;
    class MongoShell;

    /*
    ** Represents list of bson objects
    */
    class BsonWidget : public QWidget
    {
        Q_OBJECT

    public:
        BsonWidget(MongoShell *shell, QWidget *parent = NULL);
        ~BsonWidget() {}
        void setDocuments(const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo = MongoQueryInfo());

    private:
        BsonTreeWidget *_bsonTree;
        MongoShell *_shell;
    };


    /*
    ** In this thread we are running task to prepare JSON string from list of BSON objects
    */
    class JsonPrepareThread : public QThread
    {
        Q_OBJECT

    private:
        /*
        ** List of documents
        */
        QList<MongoDocumentPtr> _bsonObjects;

        UUIDEncoding _uuidEncoding;
        SupportedTimes _timeZone;
    public:
        /*
        ** Constructor
        */
        JsonPrepareThread(QList<MongoDocumentPtr> bsonObjects, UUIDEncoding uuidEncoding, SupportedTimes timeZone) : exit(false)
        {
            _bsonObjects = bsonObjects;
            _uuidEncoding = uuidEncoding;
            _timeZone = timeZone;
        }

        volatile bool exit;

    protected:

        /*
        ** Overload function
        */
        virtual void run();

    signals:
        /**
         * @brief Signals when all parts prepared
         */
        void done();

        /**
         * @brief Signals when json part is ready
         */
        void partReady(const QString &part);
    };
}
