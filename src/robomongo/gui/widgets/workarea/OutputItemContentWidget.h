#pragma once

#include <QStackedWidget>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include <vector>

namespace Robomongo
{
    class FindFrame;
    class BsonTreeWidget;
    class BsonTableView;
    class JsonPrepareThread;
    class CollectionStatsTreeWidget;
    class MongoShell;

    class OutputItemContentWidget : public QWidget
    {
        Q_OBJECT

    public:
        OutputItemContentWidget(MongoShell *shell, const QString &text);
        OutputItemContentWidget(MongoShell *shell, const std::vector<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo);
        OutputItemContentWidget(MongoShell *shell, const QString &type, const std::vector<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo);
        ~OutputItemContentWidget();

        void update(const QString &text);
        void update(const std::vector<MongoDocumentPtr> &documents);

        void setup();

        bool isTextModeSupported() const { return _isTextModeSupported; }
        bool isTreeModeSupported() const { return _isTreeModeSupported; }
        bool isCustomModeSupported() const { return _isCustomModeSupported; }
        bool isTableModeSupported() const { return _isTableModeSupported; }

        bool setTextModeSupported(bool supported) { _isTextModeSupported = supported; }
        bool setTreeModeSupported(bool supported) { _isTreeModeSupported = supported; }
        bool setCustomModeSupported(bool supported) { _isCustomModeSupported = supported; }
        bool setTableModeSupported(bool supported) { _isTableModeSupported = supported; }

        void showText();
        void showTree();
        void showCustom();
        void showTable();

        void markUninitialized();

    public Q_SLOTS:
        void jsonPrepared();
        void jsonPartReady(const QString &json);

    private:
        FindFrame *configureLogText();

        FindFrame *_log;
        BsonTreeWidget *_bson;
        BsonTableView *_bsonTable;
        CollectionStatsTreeWidget *_collectionStats;

        QString _text;
        QString _type; // type of request
        std::vector<MongoDocumentPtr> _documents;
        MongoQueryInfo _queryInfo;

        QStackedWidget *_stack;
        JsonPrepareThread *_thread;

        MongoShell *_shell;

        bool _sourceIsText; // if false - source is documents

        bool _isTextModeSupported;
        bool _isTreeModeSupported;
        bool _isTableModeSupported;
        bool _isCustomModeSupported;

        bool _isTextModeInitialized;
        bool _isTreeModeInitialized;
        bool _isTableModeInitialized;
        bool _isCustomModeInitialized;

        bool _isFirstPartRendered;
    };
}
