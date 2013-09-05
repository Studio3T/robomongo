#pragma once

#include <QWidget>
#include <QSplitter>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoQueryInfo.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"

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
        OutputItemWidget(OutputWidget *viewer, OutputItemContentWidget *output, const MongoQueryInfo &info, QWidget *parent = 0);
        OutputItemContentWidget *itemContent;
        OutputWidget *output;

        OutputItemHeaderWidget *header() const { return _header; }
        void setQueryInfo(const MongoQueryInfo &queryInfo);
        MongoQueryInfo queryInfo() const { return _queryInfo; }

    private Q_SLOTS:
        void refresh(int skip, int batchSize);
        void paging_leftClicked(int skip, int limit);
        void paging_rightClicked(int skip, int limit);

    private:
        OutputItemHeaderWidget *_header;
        MongoQueryInfo _queryInfo;
        int _initialSkip;
        int _initialLimit;
    };

}
