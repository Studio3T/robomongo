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

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"
#include "robomongo/gui/widgets/workarea/ProgressBarPopup.h"

namespace Robomongo
{
    OutputWidget::OutputWidget(MongoShell *shell, QWidget *parent) :
        QFrame(parent),
        _shell(shell),
        _splitter(NULL)
    {
        QString style = QString("Robomongo--OutputWidget { background-color: %1; border-radius: 6px; }")
            .arg(QColor("#083047").lighter(660).name());
    //            .arg("white");

        //setStyleSheet(style);

    /*    setAutoFillBackground(true);
        QPalette p(palette());
        // Set background colour to black
        p.setColor(QPalette::Background, QColor("#083047").lighter(660));
        setPalette(p);*/

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

    OutputWidget::~OutputWidget()
    {

    }

    void OutputWidget::present(const QList<MongoShellResult> &results)
    {
        clearAllParts();

        foreach (MongoShellResult shellResult, results) {
            OutputItemContentWidget *output = NULL;

            if (shellResult.documents().size() > 0) {

                if (shellResult.type().empty())
                    output = new OutputItemContentWidget(_shell, shellResult.documents(), shellResult.queryInfo());
                else
                    output = new OutputItemContentWidget(_shell, QtUtils::toQString(shellResult.type()), shellResult.documents(), shellResult.queryInfo());
            } else {
                output = new OutputItemContentWidget(_shell, QtUtils::toQString(shellResult.response()));
            }

            OutputItemWidget *result = new OutputItemWidget(this, output, shellResult.queryInfo());
            ViewMode viewMode = AppRegistry::instance().settingsManager()->viewMode();

            if (viewMode == Custom) {
                if (output->isCustomModeSupported())
                    result->header()->showCustom();
                else if (output->isTreeModeSupported())
                    result->header()->showTree();
                else if (output->isTextModeSupported())
                    result->header()->showText();
            } else if (viewMode == Tree) {
                if (output->isTreeModeSupported())
                    result->header()->showTree();
                else if (output->isTextModeSupported())
                    result->header()->showText();
            } else if (viewMode == Table) {
                if (output->isTableModeSupported())
                    result->header()->showTable();
                else if (output->isTextModeSupported())
                    result->header()->showText();
            }
            else
                result->header()->showText();

            double secs = shellResult.elapsedMs() / (double) 1000;

            result->header()->setTime(QString("%1 sec.").arg(secs));

            if (!shellResult.queryInfo().isNull) {
                result->header()->setCollection(QtUtils::toQString(shellResult.queryInfo().collectionName));
                result->header()->paging()->setLimit(shellResult.queryInfo().limit);
                result->header()->paging()->setSkip(shellResult.queryInfo().skip);
            }

            _splitter->addWidget(result);
        }

        tryToMakeAllPartsEqualInSize();
    }

    void OutputWidget::updatePart(int partIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents)
    {
        if (partIndex >= _splitter->count())
            return;

        OutputItemWidget *output = (OutputItemWidget *) _splitter->widget(partIndex);

        output->header()->paging()->setSkip(queryInfo.skip);
        output->header()->paging()->setLimit(queryInfo.limit);
        output->setQueryInfo(queryInfo);
        output->itemContent->update(documents);

        if (!output->header()->treeMode())
            output->header()->showText();
        else
            output->header()->showTree();
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
            OutputItemWidget *widget = (OutputItemWidget *) _splitter->widget(i);
            widget->header()->showTree();
        }
    }

    void OutputWidget::enterTextMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemWidget *widget = (OutputItemWidget *) _splitter->widget(i);
            widget->header()->showText();
        }
    }

    void OutputWidget::enterCustomMode()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemWidget *widget = (OutputItemWidget *) _splitter->widget(i);
            widget->header()->showCustom();
        }
    }

    void OutputWidget::maximizePart(OutputItemWidget *result)
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemWidget *widget = (OutputItemWidget *) _splitter->widget(i);

            if (widget != result)
                widget->hide();
        }
    }

    void OutputWidget::restoreSize()
    {
        int count = _splitter->count();
        for (int i = 0; i < count; i++) {
            OutputItemWidget *widget = (OutputItemWidget *) _splitter->widget(i);
            widget->show();
        }
    }

    int OutputWidget::resultIndex(OutputItemWidget *result)
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
        QTimer::singleShot(100, _progressBarPopup, SLOT(hide()));
    }

    void OutputWidget::clearAllParts()
    {
        while (_splitter->count() > 0) {
            QWidget *widget = _splitter->widget(0);
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
