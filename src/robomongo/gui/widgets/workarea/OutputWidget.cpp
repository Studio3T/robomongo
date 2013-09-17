#include "robomongo/gui/widgets/workarea/OutputWidget.h"

#include <QHBoxLayout>
#include <QSplitter>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/ProgressBarPopup.h"

namespace Robomongo
{
    OutputWidget::OutputWidget(QWidget *parent) :
        QFrame(parent),
        _splitter(NULL)
    {
        _splitter = new QSplitter;
        _splitter->setOrientation(Qt::Vertical);
        _splitter->setHandleWidth(1);
        _splitter->setContentsMargins(0, 0, 0, 0);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(_splitter);
        setLayout(layout);

        _progressBarPopup = new ProgressBarPopup(this);
    }

    void OutputWidget::present(MongoShell *shell, const std::vector<MongoShellResult> &results)
    {
        std::vector<ViewMode> prev = clearAllParts();
        int count = results.size();

        for (int i=0; i<count; ++i) {
            MongoShellResult shellResult = results[i];
            OutputItemContentWidget *output = NULL;

            double secs = shellResult.elapsedMs() / 1000.f;
            ViewMode viewMode = AppRegistry::instance().settingsManager()->viewMode();
            if (prev.size()){
                viewMode = prev.back();
                prev.pop_back();
            }

            if (shellResult.documents().size() > 0) {
                output = new OutputItemContentWidget(this, viewMode, shell, QtUtils::toQString(shellResult.type()), shellResult.documents(), shellResult.queryInfo(),secs);
            } else {
                output = new OutputItemContentWidget(this, viewMode, shell, QtUtils::toQString(shellResult.response()),secs);
            }
            VERIFY(connect(output, SIGNAL(maximizedPart()), this, SLOT(maximizePart())));
            VERIFY(connect(output, SIGNAL(restoredSize()), this, SLOT(restoreSize())));
            _splitter->addWidget(output);
        }

        tryToMakeAllPartsEqualInSize();
    }

    void OutputWidget::updatePart(int partIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents)
    {
        if (partIndex >= _splitter->count())
            return;

        OutputItemContentWidget *output = (OutputItemContentWidget *) _splitter->widget(partIndex);
        output->update(queryInfo,documents);
        output->refreshOutputItem();
    }

    void OutputWidget::toggleOrientation()
    {
        if (_splitter->orientation() == Qt::Horizontal)
            _splitter->setOrientation(Qt::Vertical);
        else
            _splitter->setOrientation(Qt::Horizontal);
    }

    void OutputWidget::enterTreeMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->showTree();
        }
    }

    void OutputWidget::enterTextMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->showText();
        }
    }

    void OutputWidget::enterTableMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->showTable();
        }
    }

    void OutputWidget::enterCustomMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->showCustom();
        }
    }

    void OutputWidget::maximizePart()
    {
        OutputItemContentWidget *result = qobject_cast<OutputItemContentWidget *>(sender());
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);

            if (widget != result)
                widget->hide();
        }
    }

    void OutputWidget::restoreSize()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->show();
        }
    }

    int OutputWidget::resultIndex(OutputItemContentWidget *result)
    {
        return _splitter->indexOf(result);
    }

    void OutputWidget::showProgress()
    {
        _progressBarPopup->move(width() / 2 - 84, height() / 2 - 25);
        _progressBarPopup->show();
    }

    void OutputWidget::hideProgress()
    {
        _progressBarPopup->hide();
    }

    std::vector<ViewMode> OutputWidget::clearAllParts()
    {
        std::vector<ViewMode> res;
        while (_splitter->count() > 0) {
            OutputItemContentWidget *widget =  (OutputItemContentWidget *)_splitter->widget(_splitter->count()-1);
            res.push_back(widget->viewMode());
            widget->hide();
            delete widget;
        }
        return res;
    }

    void OutputWidget::tryToMakeAllPartsEqualInSize()
    {
        int resultsCount = _splitter->count();

        if (resultsCount <= 1)
            return;

        int dimension = _splitter->orientation() == Qt::Vertical ? _splitter->height() : _splitter->width();
        int step = dimension / resultsCount;

        QList<int> partSizes;
        for (int i = 0; i < resultsCount; ++i) {
            partSizes << step;
        }

        _splitter->setSizes(partSizes);
    }
}
