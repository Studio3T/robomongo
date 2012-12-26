#ifndef QUERYWIDGET_H
#define QUERYWIDGET_H

#include <QWidget>
#include "Core.h"
#include <QPushButton>
#include <QLineEdit>
#include <QPlainTextEdit>
#include "Qsci/qsciscintilla.h"
#include <domain/MongoShellResult.h>
#include <QLabel>


namespace Robomongo
{
    class BsonWidget;
    class EventBus;
    class DocumentListLoadedEvent;
    class ScriptExecutedEvent;
    class PlainJavaScriptEditor;
    class RoboScintilla;
    class OutputViewer;
    class WorkAreaTabWidget;

    class QueryWidget : public QWidget
    {
        Q_OBJECT

    private:

        /*
        ** Configure QsciScintilla query widget
        */
        void _configureQueryText();

        void _showPaging(bool show);

    public:
        /*
        ** Constructs query widget
        */
        QueryWidget(MongoShell *shell, WorkAreaTabWidget *tabWidget, const QString &script, QWidget * parent = NULL);

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
        ** Handle queryText linesCountChanged event
        */
        void ui_queryLinesCountChanged();

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
        void displayData(const QList<MongoShellResult> &results);

        /*
        ** Query text
        */
        RoboScintilla * _queryText;

        /*
        ** Bson widget
        */
        BsonWidget * _bsonWidget;
        OutputViewer *_viewer;
        QLabel *_outputLabel;

        /*
        ** Paging buttons
        */
        QPushButton * _leftButton;
        QPushButton * _rightButton;
        QLineEdit * _pageSizeEdit;

        WorkAreaTabWidget *_tabWidget;

        QList<MongoShellResult> _currentResults;

//        PlainJavaScriptEditor *_queryText;

        MongoShell *_shell;
        EventBus *_bus;
        App *_app;
        KeyboardManager *_keyboard;
    };
}



#endif // QUERYWIDGET_H
