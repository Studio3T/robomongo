#include "robomongo/gui/widgets/workarea/OutputWidget.h"

#include <QHBoxLayout>
#include <QSplitter>
#include <QWidget>
#include <QMouseEvent>

#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/ProgressBarPopup.h"
#include "robomongo/gui/widgets/workarea/WorkAreaTabBar.h"

namespace Robomongo
{
    OutputWidget::OutputWidget(QWidget *parent) :
        QTabWidget(parent),
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

        setTabsClosable(true);
        setElideMode(Qt::ElideRight);
        setMovable(true);
#ifdef __APPLE__      
        setDocumentMode(false);
#else        
        setDocumentMode(true);
#endif        
        setStyleSheet(buildStyleSheet());
        VERIFY(connect(this, SIGNAL(tabCloseRequested(int)), SLOT(tabCloseRequested(int))));
        
        _progressBarPopup = new ProgressBarPopup(this);
    }

    void OutputWidget::present(MongoShell *shell, const std::vector<MongoShellResult> &results)
    {
        if (_prevResultsCount > 0)
            clearAllParts();
        
        int const RESULTS_SIZE = _prevResultsCount = results.size();
        bool const multipleResults = (RESULTS_SIZE > 1);
        bool const tabbedResults = (RESULTS_SIZE > 2);
        _splitter->setHidden(tabbedResults ? true : false);
        _outputItemContentWidgets.clear();        

        while (count() > 0)
            removeTab(count()-1);

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
                                                   shellResult.documents(), shellResult.queryInfo(), secs, 
                                                   multipleResults, tabbedResults, firstItem, lastItem,
                                                   shellResult.aggrInfo(), this);
            } else {
                item = new OutputItemContentWidget(viewMode, shell, QtUtils::toQString(shellResult.response()), 
                                                   secs, multipleResults, tabbedResults, firstItem, lastItem,
                                                   shellResult.aggrInfo(), this);
            }
            VERIFY(connect(item, SIGNAL(maximizedPart()), this, SLOT(maximizePart())));
            VERIFY(connect(item, SIGNAL(restoredSize()), this, SLOT(restoreSize())));

            if (tabbedResults) {
                addTab(item, QString::fromStdString(shellResult.statementShort()));
                setTabToolTip(i, QString::fromStdString(shellResult.statement()));
            }
            else
                _splitter->addWidget(item);
             
            _outputItemContentWidgets.push_back(item);
        }
        
        tryToMakeAllPartsEqualInSize();
    }

    void OutputWidget::updatePart(int partIndex, const MongoQueryInfo &queryInfo, 
                                  const std::vector<MongoDocumentPtr> &documents)
    {
        if (partIndex >= _splitter->count())
            return;

        auto outputItemContentWidget = qobject_cast<OutputItemContentWidget*>(_splitter->widget(partIndex));
        outputItemContentWidget->updateWithInfo(queryInfo, documents);
        outputItemContentWidget->refreshOutputItem();
    }

    void OutputWidget::updatePart(int partIndex, const AggrInfo &agrrInfo, 
                                  const std::vector<MongoDocumentPtr> &documents)
    {
        if (partIndex >= _splitter->count())
            return;

        auto outputItemContentWidget = qobject_cast<OutputItemContentWidget*>(_splitter->widget(partIndex));
        outputItemContentWidget->updateWithInfo(agrrInfo, documents);
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

    void OutputWidget::tabCloseRequested(int index)
    {
        removeTab(index);
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

    bool OutputWidget::progressBarActive() const 
    {
        return _progressBarPopup->isVisible();
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

    void OutputWidget::mouseReleaseEvent(QMouseEvent * event)
    {
        if (event->button() != Qt::MidButton)
            return;

        int const tabIndex = tabBar()->tabAt(event->pos());
        removeTab(tabIndex);
        QTabWidget::mouseReleaseEvent(event);
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

    QString OutputWidget::buildStyleSheet()
    {
        QColor background = palette().window().color();
        QColor gradientZero = QColor("#ffffff"); //Qt::white;//.lighter(103);
        QColor gradientOne = background.lighter(104); //Qt::white;//.lighter(103);
        QColor gradientTwo = background.lighter(108); //.lighter(103);
        QColor selectedBorder = background.darker(103);

        QString aga1 = gradientOne.name();
        QString aga2 = gradientTwo.name();
        QString aga3 = background.name();

#ifdef __APPLE__      
        QString styles = QString(
            "QTabWidget::pane { background-color: white; }"   // This style disables default styling under Mac
            "QTabWidget::tab-bar {"
                "alignment: left;"
                "border-top-left-radius: 2px;"
                "border-top-right-radius: 2px;"
            "}"
            "QTabBar::close-button { "
                "margin-top: 2px;"              
                "image: url(:/robomongo/icons/close_2_Mac_16x16.png);"
                "width: 10px;"
                "height: 10px;"
                "}"
            "QTabBar::close-button:hover { "
                "image: url(:/robomongo/icons/close_hover_16x16.png);"
                "width: 15px;"
                "height: 15px;"
            "}"
            "QTabBar::tab:selected { "
                "background: white; /*#E1E1E1*/; "
                "color: #282828;"
            "} "
            "QTabBar::tab {"
                "color: #505050;"
                "font-size: 11px;"
                "background: %1;"
                "border-top-left-radius: 2px;"
                "border-top-right-radius: 2px;"
                "border-right: 1px solid #aaaaaa;"
                "padding: 8px 0px 5px 0px;" // top r b l
            "}"
        ).arg(QWidget::palette().color(QWidget::backgroundRole()).darker(114).name());
#else // Wind and Linux
        QString styles = QString(
            "QTabBar::close-button { "
                "image: url(:/robomongo/icons/close_2_16x16.png);"
                "width: 10px;"
                "height: 10px;"
            "}"
            "QTabBar::close-button:hover { "
                  "image: url(:/robomongo/icons/close_hover_16x16.png);"
                  "width: 15px;"
                  "height: 15px;"
            "}"
            "QTabBar::tab {"
                "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                            "stop: 0 #F0F0F0, stop: 0.4 #DEDEDE,"
                                            "stop: 0.5 #E6E6E6, stop: 1.0 #E1E1E1);"
                "border: 1px solid #C4C4C3;"
                "border-bottom-color: #B8B7B6;" // #C2C7CB same as the pane color
                "border-top-left-radius: 6px;"
                "border-top-right-radius: 6px;"
                "padding: 4px 4px 5px 8px;"
                "max-width: 200px;"
                "margin: 0px;"
                "margin-left: 1px;"
                "margin-right: -3px;"  // it should be -(tab:first:margin-left + tab:last:margin-left) to fix incorrect text elidement                
            "}"
            "QTabBar::tab:selected, QTabBar::tab:hover {"
                "/* background: qlineargradient(x1: 0, y1: 1, x2: 0, y2: 0,"
                                "stop: 0 %1, stop: 0.3 %2,"    //#fafafa, #f4f4f4
                                "stop: 0.6 %3, stop: 1.0 %4); */" //#e7e7e7, #fafafa            
                "background-color: white;"
            "}"
            "QTabBar::tab:selected {"
                "margin-top: 1px;"
                "border-color: #9B9B9B;" //
                "border-bottom-color: %4;" //#fafafa
            "}"
            "QTabBar::tab:!selected {"
                "margin-top: 2px;" // make non-selected tabs look smaller
            "}"
        ).arg(gradientZero.name(), gradientOne.name(), gradientTwo.name(), "#ffffff");
#endif            

        return styles;
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
