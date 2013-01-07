#ifndef OUTPUTWIDGET_H
#define OUTPUTWIDGET_H

#include "Core.h"
#include <QStackedWidget>

namespace Robomongo
{
    class RoboScintilla;
    class BsonWidget;
    class JsonPrepareThread;

    class OutputWidget : public QWidget
    {
        Q_OBJECT

    public:
        OutputWidget(const QString &text);
        OutputWidget(const QList<MongoDocumentPtr> &documents);
        ~OutputWidget();

        void update(const QString &text);
        void update(const QList<MongoDocumentPtr> &documents);

        void setup();

        bool isTextModeSupported() const { return _isTextModeSupported; }
        bool isTreeModeSupported() const { return _isTreeModeSupported; }
        bool isCustomModeSupported() const { return _isCustomModeSupported; }

        bool setTextModeSupported(bool supported) { _isTextModeSupported = supported; }
        bool isTreeModeSupported(bool supported) { _isTreeModeSupported = supported; }
        bool isCustomModeSupported(bool supported) { _isCustomModeSupported = supported; }

        void showText();
        void showTree();
        void showCustom();

    public slots:
        void jsonPrepared();
        void jsonPartReady(const QString &json);

    private:
        RoboScintilla *_configureLogText();
        BsonWidget *_configureBsonWidget();

        RoboScintilla *_log;
        BsonWidget *_bson;

        QString _text;
        QList<MongoDocumentPtr> _documents;

        QStackedWidget *_stack;
        JsonPrepareThread *_thread;

        bool _sourceIsText; // if false - source is documents

        bool _isTextModeSupported;
        bool _isTreeModeSupported;
        bool _isCustomModeSupported;

        bool _isTextModeInitialized;
        bool _isTreeModeInitialized;
        bool _isCustomModeInitialized;

        bool _isFirstPartRendered;
    };
}


#endif // OUTPUTWIDGET_H
