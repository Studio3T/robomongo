#include "robomongo/gui/dialogs/FunctionTextEditor.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"

using namespace Robomongo;

FunctionTextEditor::FunctionTextEditor(const QString &server, const QString &database,
                                       const MongoFunction &function, QWidget *parent) :
    QDialog(parent),
    _function(function)
{
    setMinimumWidth(700);
    setMinimumHeight(550);

    Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);
    Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), server);

    QPushButton *cancel = new QPushButton("Cancel");
    connect(cancel, SIGNAL(clicked()), this, SLOT(close()));

    QPushButton *save = new QPushButton("Save");
    save->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowRight));
    connect(save, SIGNAL(clicked()), this, SLOT(accept()));

    _nameEdit = new QLineEdit(function.name());

    _queryText = new RoboScintilla;
    _textFont = chooseTextFont();
    _configureQueryText();
    _queryText->setText(_function.code());

    QFrame *hline = new QFrame();
    hline->setFrameShape(QFrame::HLine);
    hline->setFrameShadow(QFrame::Sunken);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(2, 0, 5, 1);
    hlayout->setSpacing(0);
    hlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
    hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
    hlayout->addStretch(1);

    QHBoxLayout *bottomlayout = new QHBoxLayout();
    bottomlayout->addStretch(1);

#if defined(Q_OS_MAC)
    save->setDefault(true);
    bottomlayout->addWidget(cancel, 0, Qt::AlignRight);
    bottomlayout->addWidget(save, 0, Qt::AlignRight);
#else
    bottomlayout->addWidget(save, 0, Qt::AlignRight);
    bottomlayout->addWidget(cancel, 0, Qt::AlignRight);
#endif

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(hlayout);
    layout->addWidget(hline);
    layout->addWidget(new QLabel("Name:"));
    layout->addWidget(_nameEdit);
    layout->addWidget(new QLabel("Code:"));
    layout->addWidget(_queryText);
    layout->addLayout(bottomlayout);
    setLayout(layout);
}

void FunctionTextEditor::setCursorPosition(int line, int column)
{
    _queryText->setCursorPosition(line, column);
}

void FunctionTextEditor::setCode(const QString &code)
{
    _queryText->setText(code);
}

void FunctionTextEditor::accept()
{
    if (_nameEdit->text().isEmpty() && _queryText->text().isEmpty())
        return;

    _function.setName(_nameEdit->text());
    _function.setCode(_queryText->text());

    QDialog::accept();
}

/*
** Configure QsciScintilla query widget
*/
void FunctionTextEditor::_configureQueryText()
{
    QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
    javaScriptLexer->setFont(_textFont);

    _queryText->setAutoIndent(true);
    _queryText->setIndentationsUseTabs(false);
    _queryText->setIndentationWidth(4);
    _queryText->setUtf8(true);
    _queryText->installEventFilter(this);
    _queryText->setMarginWidth(1, 0); // to hide left gray column
    _queryText->setBraceMatching(QsciScintilla::StrictBraceMatch);
    _queryText->setFont(_textFont);
    _queryText->setPaper(QColor(255, 0, 0, 127));
    _queryText->setLexer(javaScriptLexer);
    _queryText->setCaretForegroundColor(QColor("#FFFFFF"));
    _queryText->setMatchedBraceBackgroundColor(QColor(73, 76, 78));
    _queryText->setMatchedBraceForegroundColor(QColor("#FF8861")); //1AB0A6
    _queryText->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);
    _queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _queryText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // Wrap mode turned off because it introduces huge performance problems
    // even for medium size documents.
    _queryText->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);

    _queryText->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
}

QFont FunctionTextEditor::chooseTextFont()
{
    QFont textFont = font();
#if defined(Q_OS_MAC)
    textFont.setPointSize(12);
    textFont.setFamily("Monaco");
#elif defined(Q_OS_UNIX)
    textFont.setFamily("Monospace");
    textFont.setFixedPitch(true);
#elif defined(Q_OS_WIN)
    textFont.setPointSize(font().pointSize() + 2);
    textFont.setFamily("Courier");
#endif

    return textFont;
}
