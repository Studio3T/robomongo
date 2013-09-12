#include "robomongo/gui/widgets/workarea/OutputWidget.h"

#include <QHBoxLayout>
#include <QMovie>
#include <QListView>
#include <QTreeView>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/core/domain/MongoShell.h"

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"
#include "robomongo/gui/widgets/workarea/ProgressBarPopup.h"

namespace Robomongo
{
    OutputWidget::OutputWidget(MongoShell *shell, QWidget *parent) :
        QFrame(parent),
        _shell(shell),
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

    void OutputWidget::present(const std::vector<MongoShellResult> &results)
    {
        std::vector<ViewMode> prev = clearAllParts();
        for (std::vector<MongoShellResult>::const_iterator it = results.begin(); it!=results.end(); ++it) {
            MongoShellResult shellResult = *it;
            OutputItemContentWidget *output = NULL;

            if (shellResult.documents().size() > 0) {
                output = new OutputItemContentWidget(this,_shell, QtUtils::toQString(shellResult.type()), shellResult.documents(), shellResult.queryInfo());
            } else {
                output = new OutputItemContentWidget(this,_shell, QtUtils::toQString(shellResult.response()));
            }

            VERIFY(connect(output->header(), SIGNAL(restoredSize()), this, SLOT(restoreSize())));
            VERIFY(connect(output->header(), SIGNAL(maximizedPart(OutputItemContentWidget *)), this, SLOT(maximizePart(OutputItemContentWidget *))));

            ViewMode viewMode = AppRegistry::instance().settingsManager()->viewMode();
            if (prev.size()){
               viewMode = prev.back();
               prev.pop_back();
            }
          
            if (viewMode == Custom) {
                if (output->isCustomModeSupported())
                    output->header()->showCustom();
                else if (output->isTreeModeSupported())
                    output->header()->showTree();
                else if (output->isTableModeSupported())
                    output->header()->showTable();
                else if (output->isTextModeSupported())
                    output->header()->showText();
            } else if (viewMode == Tree) {
                if (output->isTreeModeSupported())
                    output->header()->showTree();
                else if (output->isTableModeSupported())
                    output->header()->showTable();
                else if (output->isTextModeSupported())
                    output->header()->showText();
            } else if (viewMode == Table) {
                if (output->isTableModeSupported())
                    output->header()->showTable();
                else if (output->isTreeModeSupported())
                    output->header()->showTree();
                else if (output->isTextModeSupported())
                    output->header()->showText();
            }
            else
                output->header()->showText();

            double secs = shellResult.elapsedMs() / 1000.f;

            output->header()->setTime(QString("%1 sec.").arg(secs));

            if (!shellResult.queryInfo().isNull) {
                output->header()->setCollection(QtUtils::toQString(shellResult.queryInfo().collectionName));
                output->header()->paging()->setBatchSize(shellResult.queryInfo().batchSize);
                output->header()->paging()->setSkip(shellResult.queryInfo().skip);
            }

            _splitter->addWidget(output);
        }

        tryToMakeAllPartsEqualInSize();
    }

    void OutputWidget::updatePart(int partIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents)
    {
        if (partIndex >= _splitter->count())
            return;

        OutputItemContentWidget *output = (OutputItemContentWidget *) _splitter->widget(partIndex);

        output->header()->paging()->setSkip(queryInfo.skip);
        output->header()->paging()->setBatchSize(queryInfo.batchSize);
        output->setQueryInfo(queryInfo);
        output->header()->itemContent->update(documents);
        output->header()->refreshOutputItem();
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
            widget->header()->showTree();
        }
    }

    void OutputWidget::enterTextMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->header()->showText();
        }
    }

    void OutputWidget::enterTableMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->header()->showTable();
        }
    }

    void OutputWidget::enterCustomMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemContentWidget *widget = (OutputItemContentWidget *) _splitter->widget(i);
            widget->header()->showCustom();
        }
    }

    void OutputWidget::maximizePart(OutputItemContentWidget *result)
    {
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
            res.push_back(widget->header()->viewMode());
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
