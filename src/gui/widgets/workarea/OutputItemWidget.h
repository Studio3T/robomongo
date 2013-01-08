#ifndef OUTPUTITEMWIDGET_H
#define OUTPUTITEMWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <editors/PlainJavaScriptEditor.h>
#include "BsonWidget.h"
#include <domain/MongoShellResult.h>
#include "PagingWidget.h"

namespace Robomongo
{
    class OutputItemContentWidget;
    class OutputItemHeaderWidget;
    class OutputItemWidget;
    class OutputWidget;

    class OutputItemWidget : public QFrame
    {
        Q_OBJECT
    public:
        explicit OutputItemWidget(OutputWidget *viewer, OutputItemContentWidget *output, const QueryInfo &info, QWidget *parent = 0);
        OutputItemContentWidget *itemContent;
        OutputWidget *output;

        OutputItemHeaderWidget *header() const { return _header; }
        void setQueryInfo(const QueryInfo &queryInfo);
        QueryInfo queryInfo() const { return _queryInfo; }

    private slots:
        void paging_leftClicked(int skip, int limit);
        void paging_rightClicked(int skip, int limit);

    private:
        OutputItemHeaderWidget *_header;
        QueryInfo _queryInfo;
    };

}

#endif // OUTPUTITEMWIDGET_H
