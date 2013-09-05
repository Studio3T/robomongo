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
        _queryInfo(info),
        _initialSkip(_queryInfo.skip),
        _initialLimit(_queryInfo.limit)
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
        VERIFY(connect(_header->paging(), SIGNAL(leftClicked(int,int)), this, SLOT(paging_leftClicked(int,int))));
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

        refresh(s, limit);
    }

    void OutputItemWidget::paging_rightClicked(int skip, int limit)
    {
        skip += limit;
        refresh(skip, limit);
    }

    void OutputItemWidget::refresh(int skip, int batchSize)
    {
        // Cannot set skip lower than in the text query
        if (skip < _initialSkip) {
            _header->paging()->setSkip(_initialSkip);
            skip = _initialSkip;
        }

        int skipDelta = skip - _initialSkip;
        int limit = batchSize;

        // If limit is set to 0 it means UNLIMITED number of documents (limited only by batch size)
        // This is according to MongoDB documentation.
        if (_initialLimit != 0) {
            limit = _initialLimit - skipDelta;
            if (limit <= 0)
                limit = -1; // It means that we do not need to load documents

            if (limit > batchSize)
                limit = batchSize;
        }

        MongoQueryInfo info(_queryInfo);
        info.limit = limit;
        info.skip = skip;
        info.batchSize = batchSize;

        output->showProgress();
        output->shell()->query(output->resultIndex(this), info);
    }
}
