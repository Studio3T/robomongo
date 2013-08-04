#pragma once

#include <QStackedWidget>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class FindFrame;
    class BsonWidget;
    class JsonPrepareThread;
    class CollectionStatsTreeWidget;
    class MongoShell;

    class OutputItemContentWidget : public QWidget
    {
        Q_OBJECT

    public:
        OutputItemContentWidget(MongoShell *shell, const QString &text);
        OutputItemContentWidget(MongoShell *shell, const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo);
        OutputItemContentWidget(MongoShell *shell, const QString &type, const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo);
        ~OutputItemContentWidget();

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

        void markUninitialized();

    public slots:
        void jsonPrepared();
        void jsonPartReady(const QString &json);

    private:
        FindFrame *configureLogText();
        BsonWidget *configureBsonWidget();

        FindFrame *_log;
        BsonWidget *_bson;
        CollectionStatsTreeWidget *_collectionStats;

        QString _text;
        QString _type; // type of request
        QList<MongoDocumentPtr> _documents;
        MongoQueryInfo _queryInfo;

        QStackedWidget *_stack;
        JsonPrepareThread *_thread;

        MongoShell *_shell;

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
