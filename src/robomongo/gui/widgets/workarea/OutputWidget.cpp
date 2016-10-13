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
        _splitter(new QSplitter)
    {
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
        if (_prevResultsCount > 0) {
            clearAllParts();
        }
        int const RESULTS_SIZE = _prevResultsCount = results.size();
        bool const multipleResults = (RESULTS_SIZE > 1);
        
        _outputItemContentWidgets.clear();

        for (int i = 0; i < RESULTS_SIZE; ++i) {
            MongoShellResult shellResult = results[i];

            double secs = shellResult.elapsedMs() / 1000.f;
            ViewMode viewMode = AppRegistry::instance().settingsManager()->viewMode();
            if (_prevViewModes.size()) {
                viewMode = _prevViewModes.back();
                _prevViewModes.pop_back();
            }

            bool const firstItem = (0 == i);
            bool const lastItem = (RESULTS_SIZE-1 == i);

            OutputItemContentWidget* item = nullptr;
            if (shellResult.documents().size() > 0) {
                item = new OutputItemContentWidget(viewMode, shell, QtUtils::toQString(shellResult.type()),
                                                   shellResult.documents(), shellResult.queryInfo(), secs, multipleResults,
                                                   firstItem, lastItem, this);
            } else {
                item = new OutputItemContentWidget(viewMode, shell, QtUtils::toQString(shellResult.response()), secs,
                                                   multipleResults, firstItem, lastItem, this);
            }
            VERIFY(connect(item, SIGNAL(maximizedPart()), this, SLOT(maximizePart())));
            VERIFY(connect(item, SIGNAL(restoredSize()), this, SLOT(restoreSize())));
            _splitter->addWidget(item);
            _outputItemContentWidgets.push_back(item);
        }
        
        tryToMakeAllPartsEqualInSize();
    }

    void OutputWidget::updatePart(int partIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents)
    {
        if (partIndex >= _splitter->count())
            return;

        auto outputItemContentWidget = qobject_cast<OutputItemContentWidget*>(_splitter->widget(partIndex));
        outputItemContentWidget->update(queryInfo, documents);
        outputItemContentWidget->refreshOutputItem();
    }

    void OutputWidget::toggleOrientation()
    {
        bool const horizontal = _splitter->orientation() == Qt::Horizontal;
        _splitter->setOrientation(horizontal ? Qt::Vertical : Qt::Horizontal);
        int const COUNT = _splitter->count();
        if (COUNT > 1) {
            auto const* firstItem = qobject_cast<OutputItemContentWidget*>(_splitter->widget(0));
            auto const* lastItem = qobject_cast<OutputItemContentWidget*>(_splitter->widget(COUNT-1));
            firstItem->toggleOrientation(_splitter->orientation());
            lastItem->toggleOrientation(_splitter->orientation());
        }
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
        QSize siz = size();
        QPoint point(siz.width() / 2 - ProgressBarPopup::width/2, siz.height() / 2 - ProgressBarPopup::height/2);
        _progressBarPopup->move(point);
        _progressBarPopup->show();
    }

    void OutputWidget::hideProgress()
    {
        _progressBarPopup->hide();
    }

    void OutputWidget::applyDockUndockSettings(bool isDocking) const
    {
        for (auto const& item : _outputItemContentWidgets) {
            item->applyDockUndockSettings(isDocking);
        }
    }

    Qt::Orientation OutputWidget::getOrientation() const
    {
        return _splitter->orientation();
    }

    void OutputWidget::clearAllParts()
    {
        _prevViewModes.clear();
        while (_splitter->count() > 0) {
            OutputItemContentWidget *widget =  (OutputItemContentWidget *)_splitter->widget(_splitter->count()-1);
            _prevViewModes.push_back(widget->viewMode());
            widget->hide();
            delete widget;
        }
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
