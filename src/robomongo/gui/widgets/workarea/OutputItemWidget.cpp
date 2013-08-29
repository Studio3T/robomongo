#include "robomongo/gui/widgets/workarea/OutputItemWidget.h"

#include <QVBoxLayout>

#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{

    OutputItemWidget::OutputItemWidget(OutputWidget *viewer, OutputItemContentWidget *output, const MongoQueryInfo &info, QWidget *parent) :
        itemContent(output),
        output(viewer),
        _queryInfo(info)
    {
        setContentsMargins(0, 0, 0, 0);
        _header = new OutputItemHeaderWidget(this, output);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->setContentsMargins(0, 1, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(_header);
        layout->addWidget(output, 1);
        setLayout(layout);

        VERIFY(connect(_header->paging(), SIGNAL(refreshed(int,int)), this, SLOT(refresh(int,int))));
        VERIFY(connect(_header->paging(), SIGNAL(rightClicked(int,int)), this, SLOT(paging_rightClicked(int,int))));
        VERIFY(connect(_header->paging(), SIGNAL(rightClicked(int,int)), this, SLOT(paging_rightClicked(int,int))));
    }

    void OutputItemWidget::setQueryInfo(const MongoQueryInfo &queryInfo)
    {
        _queryInfo = queryInfo;
    }

    void OutputItemWidget::paging_leftClicked(int skip, int limit)
    {
        int s = skip - limit;

        if (s < 0)
            s = 0;

        refresh(s,limit);
    }

    void OutputItemWidget::paging_rightClicked(int skip, int limit)
    {
        skip += limit;
        refresh(skip,limit);
    }

    void OutputItemWidget::refresh(int skip, int limit)
    {
        MongoQueryInfo info(_queryInfo);
        info.limit = limit;
        info.skip = skip;

        output->showProgress();
        output->shell()->query(output->resultIndex(this), info);
    }
}
