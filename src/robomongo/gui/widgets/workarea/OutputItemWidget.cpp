#include "robomongo/gui/widgets/workarea/OutputItemWidget.h"

#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/gui/widgets/workarea/OutputWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"

using namespace Robomongo;

OutputItemWidget::OutputItemWidget(OutputWidget *viewer, OutputItemContentWidget *output, const QueryInfo &info, QWidget *parent) :
    itemContent(output),
    output(viewer),
    _queryInfo(info)
{
    setContentsMargins(0, 0, 0, 0);
    _header = new OutputItemHeaderWidget(this, output);

    QFrame *hline = new QFrame();
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 1, 0, 0);
    layout->setSpacing(0);
//    layout->addWidget(hline);
    layout->addWidget(_header);
    layout->addWidget(output, 1);
    setLayout(layout);

    connect(_header->paging(), SIGNAL(leftClicked(int,int)), this, SLOT(paging_leftClicked(int,int)));
    connect(_header->paging(), SIGNAL(rightClicked(int,int)), this, SLOT(paging_rightClicked(int,int)));

}

void OutputItemWidget::setQueryInfo(const QueryInfo &queryInfo)
{
    _queryInfo = queryInfo;
}

void OutputItemWidget::paging_leftClicked(int skip, int limit)
{
    if (limit > 50)
        limit = 50;

    int s = skip - limit;

    if (s < 0)
        s = 0;

    QueryInfo info(_queryInfo);
    info.limit = limit;
    info.skip = s;

    output->shell()->query(output->resultIndex(this), info);
}

void OutputItemWidget::paging_rightClicked(int skip, int limit)
{
    if (limit > 50)
        limit = 50;

    QueryInfo info(_queryInfo);
    info.limit = limit;
    info.skip = skip + limit;

    output->shell()->query(output->resultIndex(this), info);
}
