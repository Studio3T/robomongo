#ifndef BSONWIDGET_H
#define BSONWIDGET_H

#include <QWidget>
#include "Core.h"
#include "domain/MongoDocument.h"
#include <QtGui>

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

    private:

        QTabWidget * _modeTabs;

        BsonTreeWidget * _bsonTree;

        QList<MongoDocumentPtr> _documents;

        /*
        ** Json text text box
        */
//        QsciScintilla *_jsonText;
        QPlainTextEdit *_jsonText;

        /*
        ** Log text box
        */
//        QsciScintilla *_logText;
        QPlainTextEdit *_logText;

        /*
        ** Was text rendered, or not
        */
        bool _textRendered;

    public:
        /*
        ** Constructs BsonWidget
        */
        BsonWidget(QWidget *parent);

        ~BsonWidget();

        /*
        ** Set documents to be displayed
        */
        void setDocuments(const QList<MongoDocumentPtr> & documents);

        void setShellOutput(const QString & output);

    public slots:

        /*
        ** Handle mode tabs (Tree/Text) current page index changed event
        */
        void ui_tabPageChanged(int index);

        /*
        ** Handle moment when json prepared
        */
        void thread_jsonPrepared(const QString & json);
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
        JsonPrepareThread(QList<MongoDocumentPtr> bsonObjects)
        {
            _bsonObjects = bsonObjects;
        }

        /*
        ** Overload function
        */
        void run()
        {
            QString json = MongoDocument::buildJsonString(_bsonObjects);
            emit finished(json);
        }

    signals:
        /*
        ** Signals when json prepared
        */
        void finished(const QString & text);
    };
}


#endif // BSONWIDGET_H
