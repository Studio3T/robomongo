#pragma once

#include <QWidget>
#include <QtGui>
#include <mongo/client/dbclient.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/engine/JsonBuilder.h"

class QPlainTextEdit;

namespace Robomongo
{
    class BsonTreeWidget;

    /*
    ** Represents list of bson objects
    */
    class BsonWidget : public QWidget
    {
        Q_OBJECT

    public:
        BsonWidget(QWidget *parent = NULL);
        ~BsonWidget() {}
        void setDocuments(const QList<MongoDocumentPtr> &documents);

    private:
        BsonTreeWidget *_bsonTree;
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

    public:
        /*
        ** Constructor
        */
        JsonPrepareThread(QList<MongoDocumentPtr> bsonObjects) : exit(false)
        {
            _bsonObjects = bsonObjects;
        }

        volatile bool exit;

    protected:

        /*
        ** Overload function
        */
        void run()
        {
            int position = 0;
            foreach(MongoDocumentPtr doc, _bsonObjects)
            {
                mongo::StringBuilder sb;
                if (position == 0)
                    sb << "/* 0 */\n";
                else
                    sb << "\n\n/* " << position << "*/\n";

                // Approach #1
                // std::string stdJson = doc->bsonObj().jsonString(mongo::TenGen, 1);

                // Approach #2
                // std::string stdJson = doc->bsonObj().toString(false, true);

                JsonBuilder builder;
                mongo::BSONObj obj = doc->bsonObj();
                std::string stdJson = builder.jsonString(obj, mongo::TenGen, 1);

                if (exit) {
                    emit done();
                    return;
                }

                sb << stdJson;
                QString json = QString::fromUtf8(sb.str().data());

                if (exit) {
                    emit done();
                    return;
                }

                emit partReady(json);

                position++;
            }

            emit done();
        }

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
