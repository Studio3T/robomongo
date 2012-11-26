#ifndef QUERYWIDGET_H
#define QUERYWIDGET_H

#include <QWidget>
#include "Core.h"
#include <QPushButton>
#include <QLineEdit>

namespace Robomongo
{
    class BsonWidget;

    class QueryWidget : public QWidget
    {
        Q_OBJECT

    private:

        /*
        ** Query text
        */
        //QsciScintilla * _queryText;

        /*
        ** Bson widget
        */
        //BsonWidget * _bsonWidget;

        /*
        ** Paging buttons
        */
        QPushButton * _leftButton;
        QPushButton * _rightButton;
        QLineEdit * _pageSizeEdit;

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
        QueryWidget(QWidget * parent);

        /*
        ** Destructs QueryWidget
        */
        ~QueryWidget();

        /*
        ** Override event filter
        */
        bool eventFilter(QObject * o, QEvent * e);

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
    };
}



#endif // QUERYWIDGET_H
