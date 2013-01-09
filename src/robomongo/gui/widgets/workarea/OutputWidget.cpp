#include "robomongo/gui/widgets/workarea/OutputWidget.h"

#include <QHBoxLayout>
#include <QListView>
#include <QTreeView>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/MainWindow.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/widgets/workarea/OutputItemContentWidget.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemWidget.h"
#include "robomongo/gui/widgets/workarea/OutputItemHeaderWidget.h"

using namespace Robomongo;

OutputWidget::OutputWidget(bool textMode, MongoShell *shell, QWidget *parent) :
    QFrame(parent),
    _textMode(textMode),
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
    setPalette(p);
    */

    _splitter = new QSplitter;
    _splitter->setOrientation(Qt::Vertical);
    _splitter->setHandleWidth(1);
    _splitter->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(4, 0, 4, 4);
    layout->setSpacing(0);
    layout->addWidget(_splitter);
    setLayout(layout);
}

OutputWidget::~OutputWidget()
{

}

void OutputWidget::present(const QList<MongoShellResult> &results)
{
    int count = _splitter->count();

    for (int i = 0; i < count; i++) {
        QWidget *widget = _splitter->widget(i);
        widget->hide();
        widget->deleteLater();
    }

    foreach (MongoShellResult shellResult, results) {
        OutputItemContentWidget *output = NULL;

        if (!shellResult.response.trimmed().isEmpty()) {
            output = new OutputItemContentWidget(shellResult.response);
        }

        if (shellResult.documents.count() > 0) {
            output = new OutputItemContentWidget(shellResult.documents);
        }

        OutputItemWidget *result = new OutputItemWidget(this, output, shellResult.queryInfo);
        if (GuiRegistry::instance().mainWindow()->textMode())
            result->header()->showText();
        else
            result->header()->showTree();

        double secs = shellResult.elapsedms / (double) 1000;

        result->header()->setTime(QString("%1 sec.").arg(secs));

        if (!shellResult.queryInfo.isNull) {
            result->header()->setCollection(shellResult.queryInfo.collectionName);
            result->header()->paging()->setLimit(shellResult.queryInfo.limit);
            result->header()->paging()->setSkip(shellResult.queryInfo.skip);
        }

        _splitter->addWidget(result);
    }
}

void OutputWidget::updatePart(int partIndex, const QueryInfo &queryInfo, const QList<MongoDocumentPtr> &documents)
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
