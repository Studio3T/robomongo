#include "robomongo/gui/widgets/workarea/ScriptWidget.h"

#include <QFont>
#include <QVBoxLayout>
#include <QPainter>
#include <QIcon>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

using namespace Robomongo;

ScriptWidget::ScriptWidget(MongoShell *shell)
{
    setStyleSheet("QFrame {background-color: rgb(255, 255, 255); border: 0px solid #c7c5c4; border-radius: 0px; margin: 0px; padding: 0px;}");

    _queryText = new RoboScintilla;
    _topStatusBar = new TopStatusBar(shell);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(5, 1, 5, 5);
    layout->addWidget(_topStatusBar, 0, Qt::AlignTop);
    layout->addWidget(_queryText, 1, Qt::AlignTop);
    setLayout(layout);

    // Query text widget
    _configureQueryText();
    _queryText->setFixedHeight(10);
    ui_queryLinesCountChanged();

    _queryText->setCursorPosition(15, 1000);
    _queryText->setFocus();
}

void ScriptWidget::setText(const QString &text)
{
    _queryText->setText(text);
}

QString ScriptWidget::text() const
{
    return _queryText->text();
}

QString ScriptWidget::selectedText() const
{
    return _queryText->selectedText();
}

void ScriptWidget::selectAll()
{
    _queryText->selectAll();
}

void ScriptWidget::setScriptFocus()
{
    _queryText->setFocus();
}

void ScriptWidget::setCurrentDatabase(const QString &database, bool isValid)
{
    _topStatusBar->setCurrentDatabase(database, isValid);
}

void ScriptWidget::setCurrentServer(const QString &address, bool isValid)
{
    _topStatusBar->setCurrentServer(address, isValid);
}

void ScriptWidget::ui_queryLinesCountChanged()
{
    setUpdatesEnabled(false);
    int pos = _queryText->fontInfo().pointSize();
    int pis = _queryText->fontInfo().pixelSize();
    int teh = _queryText->textHeight(0);
    int exa = _queryText->extraAscent();
    int exd = _queryText->extraDescent();

    QFontMetrics m(_queryText->font());
    int lineHeight = m.lineSpacing();

#if defined(Q_OS_UNIX)
    // this fix required to calculate correct height in Linux.
    // not the best way, but for now it tested on Ubuntu.
    lineHeight++;
#endif

    int numberOfLines = _queryText->lines();

    int height = numberOfLines * lineHeight + 8;

    int maxHeight = 18 * lineHeight + 8;
    if (height > maxHeight)
        height = maxHeight;

    _queryText->setFixedHeight(height);

    // this line helps eliminate UI flicks (because of background redraw)
    layout()->activate();

    setUpdatesEnabled(true);
}

/*
** Configure QsciScintilla query widget
*/
void ScriptWidget::_configureQueryText()
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

    QsciLexerJavaScript * javaScriptLexer = new JSLexer(this);
    javaScriptLexer->setFont(textFont);
//    javaScriptLexer->setPaper(QColor(255, 0, 0, 127));


    _queryText->setFixedHeight(23);
    _queryText->setAutoIndent(true);
    _queryText->setIndentationsUseTabs(false);
    _queryText->setIndentationWidth(4);
    _queryText->setUtf8(true);
    _queryText->installEventFilter(this);
    _queryText->setMarginWidth(1, 0); // to hide left gray column
    _queryText->setBraceMatching(QsciScintilla::StrictBraceMatch);
    _queryText->setFont(textFont);
    _queryText->setPaper(QColor(255, 0, 0, 127));
    _queryText->setLexer(javaScriptLexer);
    _queryText->setCaretForegroundColor(QColor("#FFFFFF"));
    _queryText->setMatchedBraceBackgroundColor(QColor(48, 10, 36));
    _queryText->setMatchedBraceForegroundColor(QColor("#1AB0A6"));
    _queryText->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_WORD);
    _queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _queryText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    //_queryText->SendScintilla(QsciScintilla::SCI_SETFONTQUALITY, QsciScintilla::SC_EFF_QUALITY_LCD_OPTIMIZED);
    //_queryText->SendScintilla (QsciScintillaBase::SCI_SETKEYWORDS, "db");

    _queryText->setStyleSheet("QFrame { background-color: rgb(48, 10, 36); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    connect(_queryText, SIGNAL(linesChanged()), SLOT(ui_queryLinesCountChanged()));
}

void ElidedLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);

    return;
    QPainter painter(this);
    QFontMetrics metrics(font());
    QString elided = metrics.elidedText(text(), Qt::ElideRight, width());
    painter.drawText(rect(), alignment(), elided);
}

QSize ElidedLabel::minimumSizeHint() const
{
    QSize defaultMinSizeHint = QLabel::minimumSizeHint();
    return QSize(0, defaultMinSizeHint.height());
}

QSize ElidedLabel::sizeHint() const
{
    QSize defaultSizeHint = QLabel::sizeHint();
    return defaultSizeHint;
}


TopStatusBar::TopStatusBar(MongoShell *shell) :
    _shell(shell)
{
    setContentsMargins(0, 0, 0, 0);
    _textColor = palette().text().color().lighter(200);

    QIcon dbIcon = GuiRegistry::instance().databaseIcon();
    QPixmap dbPixmap = dbIcon.pixmap(16, 16, QIcon::Disabled);
    QLabel *dbIconLabel = new QLabel;
    dbIconLabel->setPixmap(dbPixmap);

    QIcon serverIcon = GuiRegistry::instance().serverIcon();
    QPixmap serverPixmap = serverIcon.pixmap(16, 16, QIcon::Disabled);
    QLabel *serverIconLabel = new QLabel;
    serverIconLabel->setPixmap(serverPixmap);

    _currentServerLabel = new ElidedLabel(QString("<font color='%1'>%2</font>").arg(_textColor.name()).arg(_shell->server()->connectionRecord()->getFullAddress()));
    _currentServerLabel->setDisabled(true);

    _currentDatabaseLabel = new ElidedLabel();
    _currentDatabaseLabel->setDisabled(true);

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(2, 7, 2, 3);
    topLayout->addWidget(serverIconLabel, 0, Qt::AlignLeft);
    topLayout->addWidget(_currentServerLabel, 0, Qt::AlignLeft);
    topLayout->addSpacing(10);
    topLayout->addWidget(dbIconLabel, 0, Qt::AlignLeft);
    topLayout->addWidget(_currentDatabaseLabel, 0, Qt::AlignLeft);
    topLayout->addStretch(1);

    setLayout(topLayout);
}

void TopStatusBar::setCurrentDatabase(const QString &database, bool isValid)
{
    QString color = isValid ? _textColor.name() : "red";

    QString text = QString("<font color='%1'>%2</font>")
            .arg(color)
            .arg(database);

    _currentDatabaseLabel->setText(text);
}

void TopStatusBar::setCurrentServer(const QString &address, bool isValid)
{
    QString color = isValid ? _textColor.name() : "red";

    QString text = QString("<font color='%1'>%2</font>")
            .arg(color)
            .arg(address);

    _currentServerLabel->setText(text);
}
