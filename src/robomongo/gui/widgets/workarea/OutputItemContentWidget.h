#pragma once

#include <QStackedWidget>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/core/Enums.h"
#include <vector>

namespace Robomongo
{
    class FindFrame;
    class BsonTreeView;
    class BsonTableView;
    class BsonTreeModel;
    class JsonPrepareThread;
    class CollectionStatsTreeWidget;
    class MongoShell;
    class OutputItemHeaderWidget;
    class OutputWidget;

    class OutputItemContentWidget : public QWidget
    {
        Q_OBJECT

    public:
        typedef QWidget BaseClass;
        OutputItemContentWidget(ViewMode viewMode, MongoShell *shell, const QString &text, double secs,
                                bool multipleResults, bool firstItem, bool lastItem, QWidget *parent);
        OutputItemContentWidget(ViewMode viewMode, MongoShell *shell, const QString &type,
                                const std::vector<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo, 
                                double secs, bool multipleResults, bool firstItem, bool lastItem, QWidget *parent);
        int _initialSkip;
        int _initialLimit;
        void update(const MongoQueryInfo &inf, const std::vector<MongoDocumentPtr> &documents);
        bool isTextModeSupported() const { return _isTextModeSupported; }
        bool isTreeModeSupported() const { return _isTreeModeSupported; }
        bool isCustomModeSupported() const { return _isCustomModeSupported; }
        bool isTableModeSupported() const { return _isTableModeSupported; }
        ViewMode viewMode() const { return _viewMode; }

        void refreshOutputItem();
        void markUninitialized();

        void applyDockUndockSettings(bool isDocking) const;
        void toggleOrientation(Qt::Orientation orientation) const;

    Q_SIGNALS:
        void restoredSize();
        void maximizedPart();

    public Q_SLOTS:
        void showText();
        void showTree();        
        void showTable();
        void showCustom();

    private Q_SLOTS:
        void jsonPartReady(const QString &json);
        void refresh(int skip, int batchSize);
        void paging_rightClicked(int skip, int batchSize);
        void paging_leftClicked(int skip, int limit);      

    private:
        void setup(double secs, bool multipleResults, bool firstItem, bool lastItem);
        FindFrame *configureLogText();
        BsonTreeModel *configureModel();

        FindFrame *_textView;
        BsonTreeView *_bsonTreeview;
        BsonTableView *_bsonTable;
        BsonTreeModel *_mod;
        CollectionStatsTreeWidget *_collectionStats;

        QString _text;
        QString _type; // type of request
        std::vector<MongoDocumentPtr> _documents;
        MongoQueryInfo _queryInfo;

        QStackedWidget *_stack;
        JsonPrepareThread *_thread;

        MongoShell *_shell;
        OutputItemHeaderWidget *_header;
        OutputWidget *_outputWidget;
        bool _isTextModeSupported;
        bool _isTreeModeSupported;
        bool _isTableModeSupported;
        bool _isCustomModeSupported;

        bool _isTextModeInitialized;
        bool _isTreeModeInitialized;
        bool _isTableModeInitialized;
        bool _isCustomModeInitialized;

        bool _isFirstPartRendered;
        ViewMode _viewMode;
    };
}
