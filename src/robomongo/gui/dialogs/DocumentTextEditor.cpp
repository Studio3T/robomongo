#include "robomongo/gui/dialogs/DocumentTextEditor.h"

#include <QHBoxLayout>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include <QPushButton>

using namespace Robomongo;

DocumentTextEditor::DocumentTextEditor(const QString &server, const QString &database, const QString &collection,
                                       const QString &json, QWidget *parent) :
    QDialog(parent)
{
    Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), collection);
    Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);
    Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), server);

    QPushButton *cancel = new QPushButton("Cancel");
    QPushButton *save = new QPushButton("Save");

    _queryText = new RoboScintilla;
    _textFont = chooseTextFont();
    _configureQueryText();
    _queryText->setText(json);

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(2, 0, 5, 1);
    hlayout->setSpacing(0);
    hlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
    hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
    hlayout->addWidget(collectionIndicator, 0, Qt::AlignLeft);
    hlayout->addStretch(1);

    QHBoxLayout *bottomlayout = new QHBoxLayout();
    bottomlayout->addStretch(1);
    bottomlayout->addWidget(save, 0, Qt::AlignRight);
    bottomlayout->addWidget(cancel, 0, Qt::AlignRight);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(hlayout);
    layout->addWidget(_queryText);
    layout->addLayout(bottomlayout);
    setLayout(layout);
}

/*
** Configure QsciScintilla query widget
*/
void DocumentTextEditor::_configureQueryText()
{
    QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
    javaScriptLexer->setFont(_textFont);

//    _queryText->setFixedHeight(23);
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
    _queryText->setMatchedBraceBackgroundColor(QColor(48, 10, 36));
    _queryText->setMatchedBraceForegroundColor(QColor("#1AB0A6"));
    _queryText->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);
    _queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _queryText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    //_queryText->SendScintilla(QsciScintilla::SCI_SETFONTQUALITY, QsciScintilla::SC_EFF_QUALITY_LCD_OPTIMIZED);
    //_queryText->SendScintilla (QsciScintillaBase::SCI_SETKEYWORDS, "db");

    _queryText->setStyleSheet("QFrame { background-color: rgb(48, 10, 36); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
//    connect(_queryText, SIGNAL(linesChanged()), SLOT(ui_queryLinesCountChanged()));
//    connect(_queryText, SIGNAL(textChanged()), SLOT(onTextChanged()));
//    connect(_queryText, SIGNAL(cursorPositionChanged(int,int)), SLOT(onCursorPositionChanged(int,int)));
}

QFont DocumentTextEditor::chooseTextFont()
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
