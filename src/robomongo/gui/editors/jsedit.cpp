/*
  This file is part of the Ofi Labs X2 project.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2010 Ariya Hidayat <ariya.hidayat@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "robomongo/gui/editors/jsedit.h"

#include <QtGui>

class JSBlockData: public QTextBlockUserData
{
public:
    QList<int> bracketPositions;
};

class JSHighlighter : public QSyntaxHighlighter
{
public:
    JSHighlighter(QTextDocument *parent = 0);
    void setColor(JSEdit::ColorComponent component, const QColor &color);
    void mark(const QString &str, Qt::CaseSensitivity caseSensitivity);

    QStringList keywords() const;
    void setKeywords(const QStringList &keywords);

protected:
    void highlightBlock(const QString &text);

private:
    QSet<QString> m_keywords;
    QSet<QString> m_knownIds;
    QHash<JSEdit::ColorComponent, QColor> m_colors;
    QString m_markString;
    Qt::CaseSensitivity m_markCaseSensitivity;
};

JSHighlighter::JSHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_markCaseSensitivity(Qt::CaseInsensitive)
{
    // default color scheme
    m_colors[JSEdit::Normal]     = QColor("#000000");
    m_colors[JSEdit::Comment]    = QColor("#808080");
    m_colors[JSEdit::Number]     = QColor("#008000");
    m_colors[JSEdit::String]     = QColor("#800000");
    m_colors[JSEdit::Operator]   = QColor("#808000");
    m_colors[JSEdit::Identifier] = QColor("#000020");
    m_colors[JSEdit::Keyword]    = QColor("#000080");
    m_colors[JSEdit::BuiltIn]    = QColor("#008080");
    m_colors[JSEdit::Marker]     = QColor("#ffff00");

    // https://developer.mozilla.org/en/JavaScript/Reference/Reserved_Words
    m_keywords << "break";
    m_keywords << "case";
    m_keywords << "catch";
    m_keywords << "continue";
    m_keywords << "default";
    m_keywords << "delete";
    m_keywords << "do";
    m_keywords << "else";
    m_keywords << "finally";
    m_keywords << "for";
    m_keywords << "function";
    m_keywords << "if";
    m_keywords << "in";
    m_keywords << "instanceof";
    m_keywords << "new";
    m_keywords << "return";
    m_keywords << "switch";
    m_keywords << "this";
    m_keywords << "throw";
    m_keywords << "try";
    m_keywords << "typeof";
    m_keywords << "var";
    m_keywords << "void";
    m_keywords << "while";
    m_keywords << "with";

    m_keywords << "true";
    m_keywords << "false";
    m_keywords << "null";

    // built-in and other popular objects + properties
    m_knownIds << "Object";
    m_knownIds << "prototype";
    m_knownIds << "create";
    m_knownIds << "defineProperty";
    m_knownIds << "defineProperties";
    m_knownIds << "getOwnPropertyDescriptor";
    m_knownIds << "keys";
    m_knownIds << "getOwnPropertyNames";
    m_knownIds << "constructor";
    m_knownIds << "__parent__";
    m_knownIds << "__proto__";
    m_knownIds << "__defineGetter__";
    m_knownIds << "__defineSetter__";
    m_knownIds << "eval";
    m_knownIds << "hasOwnProperty";
    m_knownIds << "isPrototypeOf";
    m_knownIds << "__lookupGetter__";
    m_knownIds << "__lookupSetter__";
    m_knownIds << "__noSuchMethod__";
    m_knownIds << "propertyIsEnumerable";
    m_knownIds << "toSource";
    m_knownIds << "toLocaleString";
    m_knownIds << "toString";
    m_knownIds << "unwatch";
    m_knownIds << "valueOf";
    m_knownIds << "watch";

    m_knownIds << "Function";
    m_knownIds << "arguments";
    m_knownIds << "arity";
    m_knownIds << "caller";
    m_knownIds << "constructor";
    m_knownIds << "length";
    m_knownIds << "name";
    m_knownIds << "apply";
    m_knownIds << "bind";
    m_knownIds << "call";

    m_knownIds << "String";
    m_knownIds << "fromCharCode";
    m_knownIds << "length";
    m_knownIds << "charAt";
    m_knownIds << "charCodeAt";
    m_knownIds << "concat";
    m_knownIds << "indexOf";
    m_knownIds << "lastIndexOf";
    m_knownIds << "localCompare";
    m_knownIds << "match";
    m_knownIds << "quote";
    m_knownIds << "replace";
    m_knownIds << "search";
    m_knownIds << "slice";
    m_knownIds << "split";
    m_knownIds << "substr";
    m_knownIds << "substring";
    m_knownIds << "toLocaleLowerCase";
    m_knownIds << "toLocaleUpperCase";
    m_knownIds << "toLowerCase";
    m_knownIds << "toUpperCase";
    m_knownIds << "trim";
    m_knownIds << "trimLeft";
    m_knownIds << "trimRight";

    m_knownIds << "Array";
    m_knownIds << "isArray";
    m_knownIds << "index";
    m_knownIds << "input";
    m_knownIds << "pop";
    m_knownIds << "push";
    m_knownIds << "reverse";
    m_knownIds << "shift";
    m_knownIds << "sort";
    m_knownIds << "splice";
    m_knownIds << "unshift";
    m_knownIds << "concat";
    m_knownIds << "join";
    m_knownIds << "filter";
    m_knownIds << "forEach";
    m_knownIds << "every";
    m_knownIds << "map";
    m_knownIds << "some";
    m_knownIds << "reduce";
    m_knownIds << "reduceRight";

    m_knownIds << "RegExp";
    m_knownIds << "global";
    m_knownIds << "ignoreCase";
    m_knownIds << "lastIndex";
    m_knownIds << "multiline";
    m_knownIds << "source";
    m_knownIds << "exec";
    m_knownIds << "test";

    m_knownIds << "JSON";
    m_knownIds << "parse";
    m_knownIds << "stringify";

    m_knownIds << "decodeURI";
    m_knownIds << "decodeURIComponent";
    m_knownIds << "encodeURI";
    m_knownIds << "encodeURIComponent";
    m_knownIds << "eval";
    m_knownIds << "isFinite";
    m_knownIds << "isNaN";
    m_knownIds << "parseFloat";
    m_knownIds << "parseInt";
    m_knownIds << "Infinity";
    m_knownIds << "NaN";
    m_knownIds << "undefined";

    m_knownIds << "Math";
    m_knownIds << "E";
    m_knownIds << "LN2";
    m_knownIds << "LN10";
    m_knownIds << "LOG2E";
    m_knownIds << "LOG10E";
    m_knownIds << "PI";
    m_knownIds << "SQRT1_2";
    m_knownIds << "SQRT2";
    m_knownIds << "abs";
    m_knownIds << "acos";
    m_knownIds << "asin";
    m_knownIds << "atan";
    m_knownIds << "atan2";
    m_knownIds << "ceil";
    m_knownIds << "cos";
    m_knownIds << "exp";
    m_knownIds << "floor";
    m_knownIds << "log";
    m_knownIds << "max";
    m_knownIds << "min";
    m_knownIds << "pow";
    m_knownIds << "random";
    m_knownIds << "round";
    m_knownIds << "sin";
    m_knownIds << "sqrt";
    m_knownIds << "tan";

    m_knownIds << "document";
    m_knownIds << "window";
    m_knownIds << "navigator";
    m_knownIds << "userAgent";
}

void JSHighlighter::setColor(JSEdit::ColorComponent component, const QColor &color)
{
    m_colors[component] = color;
    rehighlight();
}

void JSHighlighter::highlightBlock(const QString &text)
{
    // parsing state
    enum {
        Start = 0,
        Number = 1,
        Identifier = 2,
        String = 3,
        Comment = 4,
        Regex = 5
    };

    QList<int> bracketPositions;

    int blockState = previousBlockState();
    int bracketLevel = blockState >> 4;
    int state = blockState & 15;
    if (blockState < 0) {
        bracketLevel = 0;
        state = Start;
    }

    int start = 0;
    int i = 0;
    while (i <= text.length()) {
        QChar ch = (i < text.length()) ? text.at(i) : QChar();
        QChar next = (i < text.length() - 1) ? text.at(i + 1) : QChar();

        switch (state) {

        case Start:
            start = i;
            if (ch.isSpace()) {
                ++i;
            } else if (ch.isDigit()) {
                ++i;
                state = Number;
            } else if (ch.isLetter() || ch == '_') {
                ++i;
                state = Identifier;
            } else if (ch == '\'' || ch == '\"') {
                ++i;
                state = String;
            } else if (ch == '/' && next == '*') {
                ++i;
                ++i;
                state = Comment;
            } else if (ch == '/' && next == '/') {
                i = text.length();
                setFormat(start, text.length(), m_colors[JSEdit::Comment]);
            } else if (ch == '/' && next != '*') {
                ++i;
                state = Regex;
            } else {
                if (!QString("(){}[]").contains(ch))
                    setFormat(start, 1, m_colors[JSEdit::Operator]);
                if (ch =='{' || ch == '}') {
                    bracketPositions += i;
                    if (ch == '{')
                        bracketLevel++;
                    else
                        bracketLevel--;
                }
                ++i;
                state = Start;
            }
            break;

        case Number:
            if (ch.isSpace() || !ch.isDigit()) {
                setFormat(start, i - start, m_colors[JSEdit::Number]);
                state = Start;
            } else {
                ++i;
            }
            break;

        case Identifier:
            if (ch.isSpace() || !(ch.isDigit() || ch.isLetter() || ch == '_')) {
                QString token = text.mid(start, i - start).trimmed();
                if (m_keywords.contains(token))
                    setFormat(start, i - start, m_colors[JSEdit::Keyword]);
                else if (m_knownIds.contains(token))
                    setFormat(start, i - start, m_colors[JSEdit::BuiltIn]);
                state = Start;
            } else {
                ++i;
            }
            break;

        case String:
            if (ch == text.at(start)) {
                QChar prev = (i > 0) ? text.at(i - 1) : QChar();
                if (prev != '\\') {
                    ++i;
                    setFormat(start, i - start, m_colors[JSEdit::String]);
                    state = Start;
                } else {
                    ++i;
                }
            } else {
                ++i;
            }
            break;

        case Comment:
            if (ch == '*' && next == '/') {
                ++i;
                ++i;
                setFormat(start, i - start, m_colors[JSEdit::Comment]);
                state = Start;
            } else {
                ++i;
            }
            break;

        case Regex:
            if (ch == '/') {
                QChar prev = (i > 0) ? text.at(i - 1) : QChar();
                if (prev != '\\') {
                    ++i;
                    setFormat(start, i - start, m_colors[JSEdit::String]);
                    state = Start;
                } else {
                    ++i;
                }
            } else {
                ++i;
            }
            break;

        default:
            state = Start;
            break;
        }
    }

    if (state == Comment)
        setFormat(start, text.length(), m_colors[JSEdit::Comment]);
    else
        state = Start;

    if (!m_markString.isEmpty()) {
        int pos = 0;
        int len = m_markString.length();
        QTextCharFormat markerFormat;
        markerFormat.setBackground(m_colors[JSEdit::Marker]);
        markerFormat.setForeground(m_colors[JSEdit::Normal]);
        for (;;) {
            pos = text.indexOf(m_markString, pos, m_markCaseSensitivity);
            if (pos < 0)
                break;
            setFormat(pos, len, markerFormat);
            ++pos;
        }
    }

    if (!bracketPositions.isEmpty()) {
        JSBlockData *blockData = reinterpret_cast<JSBlockData*>(currentBlock().userData());
        if (!blockData) {
            blockData = new JSBlockData;
            currentBlock().setUserData(blockData);
        }
        blockData->bracketPositions = bracketPositions;
    }

    blockState = (state & 15) | (bracketLevel << 4);
    setCurrentBlockState(blockState);
}

void JSHighlighter::mark(const QString &str, Qt::CaseSensitivity caseSensitivity)
{
    m_markString = str;
    m_markCaseSensitivity = caseSensitivity;
    rehighlight();
}

QStringList JSHighlighter::keywords() const
{
    return m_keywords.toList();
}

void JSHighlighter::setKeywords(const QStringList &keywords)
{
    m_keywords = QSet<QString>::fromList(keywords);
    rehighlight();
}

struct BlockInfo {
    int position;
    int number;
    bool foldable: 1;
    bool folded : 1;
};

Q_DECLARE_TYPEINFO(BlockInfo, Q_PRIMITIVE_TYPE);

class SidebarWidget : public QWidget
{
public:
    SidebarWidget(JSEdit *editor);
    QVector<BlockInfo> lineNumbers;
    QColor backgroundColor;
    QColor lineNumberColor;
    QColor indicatorColor;
    QColor foldIndicatorColor;
    QFont font;
    int foldIndicatorWidth;
    QPixmap rightArrowIcon;
    QPixmap downArrowIcon;
protected:
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);
};

SidebarWidget::SidebarWidget(JSEdit *editor)
    : QWidget(editor)
    , foldIndicatorWidth(0)
{
    backgroundColor = Qt::lightGray;
    lineNumberColor = Qt::black;
    indicatorColor = Qt::white;
    foldIndicatorColor = Qt::lightGray;
}

void SidebarWidget::mousePressEvent(QMouseEvent *event)
{
    if (foldIndicatorWidth > 0) {
        int xofs = width() - foldIndicatorWidth;
        int lineNo = -1;
        int fh = fontMetrics().lineSpacing();
        int ys = event->pos().y();
        if (event->pos().x() > xofs) {
            foreach (BlockInfo ln, lineNumbers)
                if (ln.position < ys && (ln.position + fh) > ys) {
                    if (ln.foldable)
                        lineNo = ln.number;
                    break;
                }
        }
        if (lineNo >= 0) {
            JSEdit *editor = qobject_cast<JSEdit*>(parent());
            if (editor)
                editor->toggleFold(lineNo);
        }
    }
}

void SidebarWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.fillRect(event->rect(), backgroundColor);
    p.setPen(lineNumberColor);
    p.setFont(font);
    int fh = QFontMetrics(font).height();
    foreach (BlockInfo ln, lineNumbers)
        p.drawText(0, ln.position, width() - 4 - foldIndicatorWidth, fh, Qt::AlignRight, QString::number(ln.number));

    if (foldIndicatorWidth > 0) {
        int xofs = width() - foldIndicatorWidth;
        p.fillRect(xofs, 0, foldIndicatorWidth, height(), indicatorColor);

        // initialize (or recreate) the arrow icons whenever necessary
        if (foldIndicatorWidth != rightArrowIcon.width()) {
            QPainter iconPainter;
            QPolygonF polygon;

            int dim = foldIndicatorWidth;
            rightArrowIcon = QPixmap(dim, dim);
            rightArrowIcon.fill(Qt::transparent);
            downArrowIcon = rightArrowIcon;

            polygon << QPointF(dim * 0.4, dim * 0.25);
            polygon << QPointF(dim * 0.4, dim * 0.75);
            polygon << QPointF(dim * 0.8, dim * 0.5);
            iconPainter.begin(&rightArrowIcon);
            iconPainter.setRenderHint(QPainter::Antialiasing);
            iconPainter.setPen(Qt::NoPen);
            iconPainter.setBrush(foldIndicatorColor);
            iconPainter.drawPolygon(polygon);
            iconPainter.end();

            polygon.clear();
            polygon << QPointF(dim * 0.25, dim * 0.4);
            polygon << QPointF(dim * 0.75, dim * 0.4);
            polygon << QPointF(dim * 0.5, dim * 0.8);
            iconPainter.begin(&downArrowIcon);
            iconPainter.setRenderHint(QPainter::Antialiasing);
            iconPainter.setPen(Qt::NoPen);
            iconPainter.setBrush(foldIndicatorColor);
            iconPainter.drawPolygon(polygon);
            iconPainter.end();
        }

        foreach (BlockInfo ln, lineNumbers)
            if (ln.foldable) {
                if (ln.folded)
                    p.drawPixmap(xofs, ln.position, rightArrowIcon);
                else
                    p.drawPixmap(xofs, ln.position, downArrowIcon);
            }
    }
}

static int findClosingMatch(const QTextDocument *doc, int cursorPosition)
{
    QTextBlock block = doc->findBlock(cursorPosition);
    JSBlockData *blockData = reinterpret_cast<JSBlockData*>(block.userData());
    if (!blockData->bracketPositions.isEmpty()) {
        int depth = 1;
        while (block.isValid()) {
            blockData = reinterpret_cast<JSBlockData*>(block.userData());
            if (blockData && !blockData->bracketPositions.isEmpty()) {
                for (int c = 0; c < blockData->bracketPositions.count(); ++c) {
                    int absPos = block.position() + blockData->bracketPositions.at(c);
                    if (absPos <= cursorPosition)
                        continue;
                    if (doc->characterAt(absPos) == '{')
                        depth++;
                    else
                        depth--;
                    if (depth == 0)
                        return absPos;
                }
            }
            block = block.next();
        }
    }
    return -1;
}

static int findOpeningMatch(const QTextDocument *doc, int cursorPosition)
{
    QTextBlock block = doc->findBlock(cursorPosition);
    JSBlockData *blockData = reinterpret_cast<JSBlockData*>(block.userData());
    if (!blockData->bracketPositions.isEmpty()) {
        int depth = 1;
        while (block.isValid()) {
            blockData = reinterpret_cast<JSBlockData*>(block.userData());
            if (blockData && !blockData->bracketPositions.isEmpty()) {
                for (int c = blockData->bracketPositions.count() - 1; c >= 0; --c) {
                    int absPos = block.position() + blockData->bracketPositions.at(c);
                    if (absPos >= cursorPosition - 1)
                        continue;
                    if (doc->characterAt(absPos) == '}')
                        depth++;
                    else
                        depth--;
                    if (depth == 0)
                        return absPos;
                }
            }
            block = block.previous();
        }
    }
    return -1;
}

class JSDocLayout: public QPlainTextDocumentLayout
{
public:
    JSDocLayout(QTextDocument *doc);
    void forceUpdate();
};

JSDocLayout::JSDocLayout(QTextDocument *doc)
    : QPlainTextDocumentLayout(doc)
{
}

void JSDocLayout::forceUpdate()
{
    emit documentSizeChanged(documentSize());
}

class JSEditPrivate
{
public:
    JSEdit *editor;
    JSDocLayout *layout;
    JSHighlighter *highlighter;
    SidebarWidget *sidebar;
    bool showSidebar;
    bool showLineNumbers;
    bool textWrap;
    QColor cursorColor;
    bool bracketsMatching;
    QList<int> matchPositions;
    QColor bracketMatchColor;
    QList<int> errorPositions;
    QColor bracketErrorColor;
    bool codeFolding : 1;
};

JSEdit::JSEdit(QWidget *parent)
    : QPlainTextEdit(parent)
    , d_ptr(new JSEditPrivate)
{
    d_ptr->editor = this;
    d_ptr->layout = new JSDocLayout(document());
    d_ptr->highlighter = new JSHighlighter(document());
    d_ptr->sidebar = new SidebarWidget(this);
    d_ptr->showSidebar = true;
    d_ptr->showLineNumbers = true;
    d_ptr->textWrap = true;
    d_ptr->bracketsMatching = true;
    d_ptr->cursorColor = QColor(255, 255, 192);
    d_ptr->bracketMatchColor = QColor(180, 238, 180);
    d_ptr->bracketErrorColor = QColor(224, 128, 128);
    d_ptr->codeFolding = true;

    document()->setDocumentLayout(d_ptr->layout);

    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateCursor()));
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateSidebar()));
    connect(this, SIGNAL(updateRequest(QRect, int)), this, SLOT(updateSidebar(QRect, int)));

#if defined(Q_OS_MAC)
    QFont textFont = font();
    textFont.setPointSize(12);
    textFont.setFamily("Monaco");
    setFont(textFont);
#elif defined(Q_OS_UNIX)
    QFont textFont = font();
    textFont.setFamily("Monospace");
    setFont(textFont);
#elif defined(Q_OS_WIN)
    QFont textFont = font();
    textFont.setPointSize(12);
    textFont.setFamily("Courier");
    setFont(textFont);
#endif
}

JSEdit::~JSEdit()
{
    delete d_ptr->layout;
}

void JSEdit::setColor(ColorComponent component, const QColor &color)
{
    Q_D(JSEdit);

    if (component == Background) {
        QPalette pal = palette();
        pal.setColor(QPalette::Base, color);
        setPalette(pal);
        d->sidebar->indicatorColor = color;
        updateSidebar();
    } else if (component == Normal) {
        QPalette pal = palette();
        pal.setColor(QPalette::Text, color);
        setPalette(pal);
    } else if (component == Sidebar) {
        d->sidebar->backgroundColor = color;
        updateSidebar();
    } else if (component == LineNumber) {
        d->sidebar->lineNumberColor = color;
        updateSidebar();
    } else if (component == Cursor) {
        d->cursorColor = color;
        updateCursor();
    } else if (component == BracketMatch) {
        d->bracketMatchColor = color;
        updateCursor();
    } else if (component == BracketError) {
        d->bracketErrorColor = color;
        updateCursor();
    } else if (component == FoldIndicator) {
        d->sidebar->foldIndicatorColor = color;
        updateSidebar();
    } else {
        d->highlighter->setColor(component, color);
        updateCursor();
    }
}

QStringList JSEdit::keywords() const
{
    return d_ptr->highlighter->keywords();
}

void JSEdit::setKeywords(const QStringList &keywords)
{
    d_ptr->highlighter->setKeywords(keywords);
}

bool JSEdit::isLineNumbersVisible() const
{
    return d_ptr->showLineNumbers;
}

void JSEdit::setLineNumbersVisible(bool visible)
{
    d_ptr->showLineNumbers = visible;
    updateSidebar();
}

void JSEdit::setSidebarVisible(bool visible)
{
    d_ptr->showSidebar = visible;
    d_ptr->sidebar->setVisible(visible);
    updateSidebar();
}



bool JSEdit::isTextWrapEnabled() const
{
    return d_ptr->textWrap;
}

void JSEdit::setTextWrapEnabled(bool enable)
{
    d_ptr->textWrap = enable;
    setLineWrapMode(enable ? WidgetWidth : NoWrap);
}

bool JSEdit::isBracketsMatchingEnabled() const
{
    return d_ptr->bracketsMatching;
}

void JSEdit::setBracketsMatchingEnabled(bool enable)
{
    d_ptr->bracketsMatching = enable;
    updateCursor();
}

bool JSEdit::isCodeFoldingEnabled() const
{
    return d_ptr->codeFolding;
}

void JSEdit::setCodeFoldingEnabled(bool enable)
{
    d_ptr->codeFolding = enable;
    updateSidebar();
}

static int findClosingConstruct(const QTextBlock &block)
{
    if (!block.isValid())
        return -1;
    JSBlockData *blockData = reinterpret_cast<JSBlockData*>(block.userData());
    if (!blockData)
        return -1;
    if (blockData->bracketPositions.isEmpty())
        return -1;
    const QTextDocument *doc = block.document();
    int offset = block.position();
    foreach (int pos, blockData->bracketPositions) {
        int absPos = offset + pos;
        if (doc->characterAt(absPos) == '{') {
            int matchPos = findClosingMatch(doc, absPos);
            if (matchPos >= 0)
                return matchPos;
        }
    }
    return -1;
}

bool JSEdit::isFoldable(int line) const
{
    int matchPos = findClosingConstruct(document()->findBlockByNumber(line - 1));
    if (matchPos >= 0) {
        QTextBlock matchBlock = document()->findBlock(matchPos);
        if (matchBlock.isValid() && matchBlock.blockNumber() > line)
            return true;
    }
    return false;
}

bool JSEdit::isFolded(int line) const
{
    QTextBlock block = document()->findBlockByNumber(line - 1);
    if (!block.isValid())
        return false;
    block = block.next();
    if (!block.isValid())
        return false;
    return !block.isVisible();
}

void JSEdit::fold(int line)
{
    QTextBlock startBlock = document()->findBlockByNumber(line - 1);
    int endPos = findClosingConstruct(startBlock);
    if (endPos < 0)
        return;
    QTextBlock endBlock = document()->findBlock(endPos);

    QTextBlock block = startBlock.next();
    while (block.isValid() && block != endBlock) {
        block.setVisible(false);
        block.setLineCount(0);
        block = block.next();
    }

    document()->markContentsDirty(startBlock.position(), endPos - startBlock.position() + 1);
    updateSidebar();
    update();

    JSDocLayout *layout = reinterpret_cast<JSDocLayout*>(document()->documentLayout());
    layout->forceUpdate();
}

void JSEdit::unfold(int line)
{
    QTextBlock startBlock = document()->findBlockByNumber(line - 1);
    int endPos = findClosingConstruct(startBlock);

    QTextBlock block = startBlock.next();
    while (block.isValid() && !block.isVisible()) {
        block.setVisible(true);
        block.setLineCount(block.layout()->lineCount());
        endPos = block.position() + block.length();
        block = block.next();
    }

    document()->markContentsDirty(startBlock.position(), endPos - startBlock.position() + 1);
    updateSidebar();
    update();

    JSDocLayout *layout = reinterpret_cast<JSDocLayout*>(document()->documentLayout());
    layout->forceUpdate();
}

void JSEdit::toggleFold(int line)
{
    if (isFolded(line))
        unfold(line);
    else
        fold(line);
}

void JSEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    updateSidebar();
}

void JSEdit::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() == Qt::ControlModifier) {
        int steps = e->delta() / 20;
        steps = qBound(-3, steps, 3);
        QFont textFont = font();
        int pointSize = textFont.pointSize() + steps;
        pointSize = qBound(10, pointSize, 40);
        textFont.setPointSize(pointSize);
        setFont(textFont);
        updateSidebar();
        e->accept();
        return;
    }
    QPlainTextEdit::wheelEvent(e);
}

void JSEdit::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() == Qt::Key_Tab )
    {
        this->insertPlainText("    ");
        //QMessageBox::information(this, "hello", "aga", "muga");
    }
    else if ( e->key() == Qt::Key_Return )
    {
        QTextCursor cursor = textCursor();
        QString line   = cursor.block().text();

        if (line.length() == 0)
        {
            QPlainTextEdit::keyPressEvent(e);
            return;
        }

        int spaces = 0;
        while (spaces < line.length()) {
            if (!line.at(spaces).isSpace())
                break;
            spaces++;
        }

        QChar lastSymbol = line.at(line.length() - 1);
        bool indent = lastSymbol == '{' || lastSymbol == '(';

        if (indent)
            spaces += 4;

        QPlainTextEdit::keyPressEvent(e);
        this->insertPlainText(QString(" ").repeated(spaces));
    }
    else
        QPlainTextEdit::keyPressEvent(e);
}


void JSEdit::updateCursor()
{
    Q_D(JSEdit);

    if (isReadOnly()) {
        setExtraSelections(QList<QTextEdit::ExtraSelection>());
    } else {

        d->matchPositions.clear();
        d->errorPositions.clear();

        if (d->bracketsMatching && textCursor().block().userData()) {
            QTextCursor cursor = textCursor();
            int cursorPosition = cursor.position();

            if (document()->characterAt(cursorPosition) == '{') {
                int matchPos = findClosingMatch(document(), cursorPosition);
                if (matchPos < 0) {
                    d->errorPositions += cursorPosition;
                } else {
                    d->matchPositions += cursorPosition;
                    d->matchPositions += matchPos;
                }
            }

            if (document()->characterAt(cursorPosition - 1) == '}') {
                int matchPos = findOpeningMatch(document(), cursorPosition);
                if (matchPos < 0) {
                    d->errorPositions += cursorPosition - 1;
                } else {
                    d->matchPositions += cursorPosition - 1;
                    d->matchPositions += matchPos;
                }
            }
        }

        QTextEdit::ExtraSelection highlight;
        highlight.format.setBackground(d->cursorColor);
        highlight.format.setProperty(QTextFormat::FullWidthSelection, true);
        highlight.cursor = textCursor();
        highlight.cursor.clearSelection();

        QList<QTextEdit::ExtraSelection> extraSelections;
        extraSelections.append(highlight);

        for (int i = 0; i < d->matchPositions.count(); ++i) {
            int pos = d->matchPositions.at(i);
            QTextEdit::ExtraSelection matchHighlight;
            matchHighlight.format.setBackground(d->bracketMatchColor);
            matchHighlight.cursor = textCursor();
            matchHighlight.cursor.setPosition(pos);
            matchHighlight.cursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
            extraSelections.append(matchHighlight);
        }

        for (int i = 0; i < d->errorPositions.count(); ++i) {
            int pos = d->errorPositions.at(i);
            QTextEdit::ExtraSelection errorHighlight;
            errorHighlight.format.setBackground(d->bracketErrorColor);
            errorHighlight.cursor = textCursor();
            errorHighlight.cursor.setPosition(pos);
            errorHighlight.cursor.setPosition(pos + 1, QTextCursor::KeepAnchor);
            extraSelections.append(errorHighlight);
        }

        setExtraSelections(extraSelections);
    }
}

void JSEdit::updateSidebar(const QRect &rect, int d)
{
    Q_UNUSED(rect)
    if (d != 0)
        updateSidebar();
}

void JSEdit::updateSidebar()
{
    Q_D(JSEdit);

    if (!d->showSidebar)
        return;

    if (!d->showLineNumbers && !d->codeFolding) {
        d->sidebar->hide();
        setViewportMargins(0, 0, 0, 0);
        d->sidebar->setGeometry(3, 0, 0, height());
        return;
    }

    d->sidebar->foldIndicatorWidth = 0;
    d->sidebar->font = this->font();
    d->sidebar->show();

    int sw = 0;
    if (d->showLineNumbers) {
        int digits = 2;
        int maxLines = blockCount();
        for (int number = 10; number < maxLines; number *= 10)
            ++digits;
        sw += fontMetrics().width('w') * digits;
    }
    if (d->codeFolding) {
        int fh = fontMetrics().lineSpacing();
        int fw = fontMetrics().width('w');
        d->sidebar->foldIndicatorWidth = qMax(fw, fh);
        sw += d->sidebar->foldIndicatorWidth;
    }
    setViewportMargins(sw, 0, 0, 0);

    d->sidebar->setGeometry(0, 0, sw, height());
    QRectF sidebarRect(0, 0, sw, height());

    QTextBlock block = firstVisibleBlock();
    int index = 0;
    while (block.isValid()) {
        if (block.isVisible()) {
            QRectF rect = blockBoundingGeometry(block).translated(contentOffset());
            if (sidebarRect.intersects(rect)) {
                if (d->sidebar->lineNumbers.count() >= index)
                    d->sidebar->lineNumbers.resize(index + 1);
                d->sidebar->lineNumbers[index].position = rect.top();
                d->sidebar->lineNumbers[index].number = block.blockNumber() + 1;
                d->sidebar->lineNumbers[index].foldable = d->codeFolding ? isFoldable(block.blockNumber() + 1) : false;
                d->sidebar->lineNumbers[index].folded = d->codeFolding ? isFolded(block.blockNumber() + 1) : false;
                ++index;
            }
            if (rect.top() > sidebarRect.bottom())
                break;
        }
        block = block.next();
    }
    d->sidebar->lineNumbers.resize(index);
    d->sidebar->update();
}

void JSEdit::mark(const QString &str, Qt::CaseSensitivity sens)
{
    d_ptr->highlighter->mark(str, sens);
}
