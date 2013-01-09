#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include "Qsci/qsciscintilla.h"

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoShellResult.h"

namespace Robomongo
{
    class BsonWidget;
    class EventBus;
    class DocumentListLoadedEvent;
    class ScriptExecutedEvent;
    class PlainJavaScriptEditor;
    class RoboScintilla;
    class OutputWidget;
    class WorkAreaTabWidget;
    class ScriptWidget;
    class TopStatusBar;

    class QueryWidget : public QWidget
    {
        Q_OBJECT

    public:
        /*
        ** Constructs query widget
        */
        QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, const QString &script, bool textMode, QWidget * parent = NULL);

        /*
        ** Destructs QueryWidget
        */
        ~QueryWidget();

        /*
        ** Override event filter
        */
        bool eventFilter(QObject * o, QEvent * e);

        void toggleOrientation();
        void activateTabContent();
        void openNewTab();
        void reload();
        void duplicate();
        void enterTreeMode();
        void enterTextMode();

        MongoShell *shell() const { return _shell; }

    public slots:

        /*
        ** Execute query
        */
        void ui_executeButtonClicked();

        /*
        ** Paging right clicked
        */
        void ui_rightButtonClicked();

        /*
        ** Paging left clicked
        */
        void ui_leftButtonClicked();

        /*
        ** Documents refreshed
        */
        void vm_documentsRefreshed(const QList<MongoDocumentPtr> & documents);

        /*
        ** Shell output refreshed
        */
        void vm_shellOutputRefreshed(const QString & shellOutput);

        /*
        ** Paging visability changed
        */
        void vm_pagingVisibilityChanged(bool show);

        /*
        ** Query updated
        */
        void vm_queryUpdated(const QString & query);

    public slots:
        void handle(DocumentListLoadedEvent *event);
        void handle(ScriptExecutedEvent *event);

    private:

        ScriptWidget *_scriptWidget;

        void _showPaging(bool show);

        bool _textMode;

        void displayData(const QList<MongoShellResult> &results, bool empty);

        /*
        ** Bson widget
        */
        BsonWidget * _bsonWidget;
        OutputWidget *_viewer;
        QLabel *_outputLabel;

        /*
        ** Paging buttons
        */
        QPushButton * _leftButton;
        QPushButton * _rightButton;
        QLineEdit * _pageSizeEdit;

        WorkAreaTabWidget *_tabWidget;

        QList<MongoShellResult> _currentResults;

        MongoShell *_shell;
        EventBus *_bus;
        App *_app;
        KeyboardManager *_keyboard;

        /**
         * @brief QueryWidget initialized and connected to the shell
         */
        bool _initialized;
    };
}
