#pragma once

#include <QStackedWidget>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include <vector>

namespace Robomongo
{
    class FindFrame;
    class BsonTreeView;
    class BsonTableView;
    class JsonPrepareThread;
    class CollectionStatsTreeWidget;
    class MongoShell;

    class OutputItemContentWidget : public QWidget
    {
        Q_OBJECT

    public:
        OutputItemContentWidget(MongoShell *shell, const QString &text);
        OutputItemContentWidget(MongoShell *shell, const QString &type, const std::vector<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo);

        void update(const std::vector<MongoDocumentPtr> &documents);

        bool isTextModeSupported() const { return _isTextModeSupported; }
        bool isTreeModeSupported() const { return _isTreeModeSupported; }
        bool isCustomModeSupported() const { return _isCustomModeSupported; }
        bool isTableModeSupported() const { return _isTableModeSupported; }

        void showText();
        void showTree();        
        void showTable();
        void showCustom();

        void markUninitialized();

    public Q_SLOTS:
        void jsonPartReady(const QString &json);

    private:
        void setup();
        FindFrame *configureLogText();

        FindFrame *_textView;
        BsonTreeView *_bsonTreeview;
        BsonTableView *_bsonTable;
        CollectionStatsTreeWidget *_collectionStats;

        QString _text;
        QString _type; // type of request
        std::vector<MongoDocumentPtr> _documents;
        MongoQueryInfo _queryInfo;

        QStackedWidget *_stack;
        JsonPrepareThread *_thread;

        MongoShell *_shell;

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
