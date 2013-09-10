#include "robomongo/gui/widgets/workarea/ScriptWidget.h"

#include <QVBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QMovie>
#include <QCompleter>
#include <QStringListModel>
#include <QMessageBox>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciscintilla.h>

#include "robomongo/core/domain/MongoShell.h"
#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/utils/QtUtils.h"

#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"

namespace
{
    bool isStopChar(const QChar &ch, bool direction)
    {
        if (ch == '='  ||  ch == ';'  ||
            ch == '('  ||  ch == ')'  ||
            ch == '{'  ||  ch == '}'  ||
            ch == '-'  ||  ch == '/'  ||
            ch == '+'  ||  ch == '*'  ||
            ch == '\r' ||  ch == '\n' ||
            ch == ' ' ) {
                return true;
        }

        if (direction) { // right direction
            if (ch == '.')
                return true;
        }

        return false;
    }

    bool isForbiddenChar(const QChar &ch)
    {
        return ch == '\"' ||  ch == '\'';
    }
}

namespace Robomongo
{
    ScriptWidget::ScriptWidget(MongoShell *shell) :
        _shell(shell),
        _textChanged(false),
        _disableTextAndCursorNotifications(false)
    {
        setStyleSheet("QFrame {background-color: rgb(255, 255, 255); border: 0px solid #c7c5c4; border-radius: 0px; margin: 0px; padding: 0px;}");

        _queryText = new FindFrame(this);
        _topStatusBar = new TopStatusBar(shell);

        QVBoxLayout *layout = new QVBoxLayout;
        layout->setSpacing(0);
        layout->setContentsMargins(5, 1, 5, 5);
        layout->addWidget(_topStatusBar, 0, Qt::AlignTop);
        layout->addWidget(_queryText, 1, Qt::AlignTop);
        setLayout(layout);

        // Query text widget
        configureQueryText();
        _queryText->sciScintilla()->setFocus();

        _queryText->sciScintilla()->installEventFilter(this);

        _completer = new QCompleter(this);
        _completer->setWidget(_queryText->sciScintilla());
        _completer->setCompletionMode(QCompleter::PopupCompletion);
        _completer->setCaseSensitivity(Qt::CaseInsensitive);
        _completer->setMaxVisibleItems(20);
        _completer->setWrapAround(false);
        _completer->popup()->setFont(GuiRegistry::instance().font());
        VERIFY(connect(_completer, SIGNAL(activated(const QString &)), this, SLOT(onCompletionActivated(const QString&))));

        QStringListModel *model = new QStringListModel(_completer);
        _completer->setModel(model);

        setText(QtUtils::toQString(shell->query()));
        setTextCursor(shell->cursor());
    }

    bool ScriptWidget::eventFilter(QObject *obj, QEvent *event)
    {
        if (obj == _queryText->sciScintilla()) {
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter
                        || keyEvent->key() == Qt::Key_Tab) {
                    hideAutocompletion();
                    return false;
                }                
            }            
        }
        return QFrame::eventFilter(obj, event);
    }

    void ScriptWidget::setup(const MongoShellExecResult &execResult)
    {
        setCurrentDatabase(execResult.currentDatabase(), execResult.isCurrentDatabaseValid());
        setCurrentServer(execResult.currentServer(), execResult.isCurrentServerValid());
    }

    void ScriptWidget::setText(const QString &text)
    {
        _queryText->sciScintilla()->setText(text);
    }

    void ScriptWidget::setTextCursor(const CursorPosition &cursor)
    {
        if (cursor.isNull()) {
            _queryText->sciScintilla()->setCursorPosition(15, 1000);
            return;
        }

        int column = cursor.column();
        if (column < 0) {
            column = _queryText->sciScintilla()->text(cursor.line()).length() + column;
        }

        _queryText->sciScintilla()->setCursorPosition(cursor.line(), column);
    }

    QString ScriptWidget::text() const
    {
        return _queryText->sciScintilla()->text();
    }

    QString ScriptWidget::selectedText() const
    {
        return _queryText->sciScintilla()->selectedText();
    }

    void ScriptWidget::selectAll()
    {
        _queryText->sciScintilla()->selectAll();
    }

    void ScriptWidget::setScriptFocus()
    {
        _queryText->sciScintilla()->setFocus();
    }

    void ScriptWidget::setCurrentDatabase(const std::string &database, bool isValid)
    {
        _topStatusBar->setCurrentDatabase(database, isValid);
    }

    void ScriptWidget::setCurrentServer(const std::string &address, bool isValid)
    {
        _topStatusBar->setCurrentServer(address, isValid);
    }

    void ScriptWidget::showProgress()
    {
        _topStatusBar->showProgress();
    }

    void ScriptWidget::hideProgress()
    {
        _topStatusBar->hideProgress();
    }

    void ScriptWidget::showAutocompletion(const QStringList &list, const QString &prefix)
    {
        // do not show single autocompletion which is identical to existing prefix
        // or if it identical to prefix + '('.
        if (list.count() == 1) {
            if (list.at(0) == prefix ||
                list.at(0) == (prefix + "(")) {
                return;
            }
        }

        // update list of completions
        QStringListModel * model = static_cast<QStringListModel *>(_completer->model());
        model->setStringList(list);

        int currentLine = 0;
        int currentIndex = 0;
        _queryText->sciScintilla()->getCursorPosition(&currentLine, &currentIndex);
        int physicalLine = currentLine - _queryText->sciScintilla()->firstVisibleLine(); // "physical" line number in text editor (not logical)
        int lineIndexLeft = _currentAutoCompletionInfo.lineIndexLeft();

        QRect rect = _queryText->sciScintilla()->rect();
        rect.setWidth(550);
        rect.setHeight(editorHeight(physicalLine + 1));
        rect.moveLeft(charWidth() * lineIndexLeft
            + autocompletionBoxLeftPosition()
            + _queryText->sciScintilla()->lineNumberMarginWidth());

        _completer->complete(rect);
        _completer->popup()->setCurrentIndex(_completer->completionModel()->index(0, 0));
        static_cast<RoboScintilla*>(_queryText->sciScintilla())->setIgnoreEnterKey(true);
        static_cast<RoboScintilla*>(_queryText->sciScintilla())->setIgnoreTabKey(true);
    }

    void ScriptWidget::showAutocompletion()
    {
        _currentAutoCompletionInfo = sanitizeForAutocompletion();

        if (_currentAutoCompletionInfo.isEmpty()) {
            hideAutocompletion();
            return;
        }

        _shell->autocomplete(QtUtils::toStdString<std::string>(_currentAutoCompletionInfo.text()));
    }

    void ScriptWidget::hideAutocompletion()
    {
        _completer->popup()->hide();
        static_cast<RoboScintilla*>(_queryText->sciScintilla())->setIgnoreEnterKey(false);
        static_cast<RoboScintilla*>(_queryText->sciScintilla())->setIgnoreTabKey(false);
    }

    void ScriptWidget::ui_queryLinesCountChanged()
    {
        int lines = _queryText->sciScintilla()->lines();
        int height = editorHeight(lines);

        int maxHeight = editorHeight(18);
        if (height > maxHeight)
            height = maxHeight;
        _queryText->setFixedHeight(height);
        _queryText->sciScintilla()->setFixedHeight(height);
        _queryText->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
        _queryText->setMaximumHeight(height+FindFrame::HeightFindPanel);
    }

    void ScriptWidget::onTextChanged()
    {
        emit textChanged();
        if (!_disableTextAndCursorNotifications)
            _textChanged = true;
    }

    void ScriptWidget::onCursorPositionChanged(int line, int index)
    {
        if (!_disableTextAndCursorNotifications&&_textChanged){
            showAutocompletion();
            _textChanged = false;
        }
    }

    void ScriptWidget::onCompletionActivated(const QString &text)
    {
        int row = _currentAutoCompletionInfo.line();
        int colLeft = _currentAutoCompletionInfo.lineIndexLeft();
        int colRight = _currentAutoCompletionInfo.lineIndexRight();
        QString line = _queryText->sciScintilla()->text(row);

        int selectionIndexRight = colRight + 1;

        // overwrite open parenthesis, if it already exists in text
        if (text.endsWith('(')) {
            if (line.length() > colRight + 1) {
                if (line.at(colRight + 1) == '(') {
                    ++selectionIndexRight;
                }
            }
        }

        _disableTextAndCursorNotifications = true;

        _queryText->sciScintilla()->setSelection(row, colLeft, row, selectionIndexRight);
        _queryText->sciScintilla()->replaceSelectedText(text);

        _disableTextAndCursorNotifications = false;
    }

    /*
    ** Configure QsciScintilla query widget
    */
    void ScriptWidget::configureQueryText()
    {
        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        javaScriptLexer->setFont(GuiRegistry::instance().font());
        int height = editorHeight(1);
        _queryText->sciScintilla()->setMinimumHeight(height);
        _queryText->sciScintilla()->setFixedHeight(height);
        _queryText->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
        _queryText->sciScintilla()->setFont(GuiRegistry::instance().font());
        _queryText->sciScintilla()->setPaper(QColor(255, 0, 0, 127));
        _queryText->sciScintilla()->setLexer(javaScriptLexer);

        _queryText->sciScintilla()->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
        VERIFY(connect(_queryText->sciScintilla(), SIGNAL(linesChanged()), SLOT(ui_queryLinesCountChanged())));
        VERIFY(connect(_queryText->sciScintilla(), SIGNAL(textChanged()), SLOT(onTextChanged())));
        VERIFY(connect(_queryText->sciScintilla(), SIGNAL(cursorPositionChanged(int,int)), SLOT(onCursorPositionChanged(int,int))));
    }

    /**
     * @brief Calculates line height of text editor
     */
    int ScriptWidget::lineHeight() const
    {  
        return _queryText->sciScintilla()->textHeight(-1);
    }

    /**
     * @brief Calculates char width of text editor
     */
    int ScriptWidget::charWidth()
    {
        QFontMetrics m(_queryText->sciScintilla()->font());
        return m.averageCharWidth();
    }

    int ScriptWidget::autocompletionBoxLeftPosition()
    {
    #if defined(Q_OS_MAC)
        return -1;
    #endif
        // for Linux and Windows it is the same for now
        return 1;
    }

    /**
     * @brief Calculates preferable editor height for specified number of lines
     */
    int ScriptWidget::editorHeight(int lines) const
    {
        return lines * lineHeight() + 8;
    }

    AutoCompletionInfo ScriptWidget::sanitizeForAutocompletion()
    {
        int row = 0;
        int col = 0;
        _queryText->sciScintilla()->getCursorPosition(&row, &col);
        QString line = _queryText->sciScintilla()->text(row);

        int leftStop = -1;
        for (int i = col - 1; i >= 0; --i) {
            const QChar ch = line.at(i);

            if (isForbiddenChar(ch))
                return AutoCompletionInfo();

            if (isStopChar(ch, false)) {
                leftStop = i;
                break;
            }
        }

        int rightStop = line.length() + 1;
        for (int i = col; i < line.length(); ++i) {
            const QChar ch = line.at(i);

            if (isForbiddenChar(ch))
                return AutoCompletionInfo();

            if (isStopChar(ch, true)) {
                rightStop = i;
                break;
            }
        }

        leftStop = leftStop + 1;
        rightStop = rightStop - 1;
        //int len = ondemand ? col - leftStop : rightStop - leftStop + 1;
        int len = col - leftStop;

        QString final = line.mid(leftStop, len);
        return AutoCompletionInfo(final, row, leftStop, rightStop);
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

        _currentServerLabel = new QLabel(QString("<font color='%1'>%2</font>").arg(_textColor.name()).arg(QtUtils::toQString(_shell->server()->connectionRecord()->getFullAddress())),this);
        _currentServerLabel->setDisabled(true);

        _currentDatabaseLabel = new QLabel(this);
        _currentDatabaseLabel->setDisabled(true);

        QMovie *movie = new QMovie(":robomongo/icons/loading.gif", QByteArray(), this);
        _progressLabel = new QLabel(this);
        _progressLabel->setMovie(movie);
        _progressLabel->hide();
        movie->start();

        QHBoxLayout *topLayout = new QHBoxLayout;
    #if defined(Q_OS_MAC)
        topLayout->setContentsMargins(2, 3, 2, 3);
    #else
        topLayout->setContentsMargins(2, 7, 2, 3);
    #endif
        topLayout->addWidget(serverIconLabel, 0, Qt::AlignLeft);
        topLayout->addWidget(_currentServerLabel, 0, Qt::AlignLeft);
        topLayout->addSpacing(10);
        topLayout->addWidget(dbIconLabel, 0, Qt::AlignLeft);
        topLayout->addWidget(_currentDatabaseLabel, 0, Qt::AlignLeft);
        topLayout->addStretch(1);
        topLayout->addWidget(_progressLabel, 0, Qt::AlignLeft);

        setLayout(topLayout);
    }

    void TopStatusBar::setCurrentDatabase(const std::string &database, bool isValid)
    {
        QString color = isValid ? _textColor.name() : "red";

        QString text = QString("<font color='%1'>%2</font>")
                .arg(color)
                .arg(database.c_str());

        _currentDatabaseLabel->setText(text);
    }

    void TopStatusBar::setCurrentServer(const std::string &address, bool isValid)
    {
        QString color = isValid ? _textColor.name() : "red";

        QString text = QString("<font color='%1'>%2</font>")
                .arg(color)
                .arg(address.c_str());

        _currentServerLabel->setText(text);
    }

    void TopStatusBar::showProgress()
    {
        _progressLabel->show();
    }

    void TopStatusBar::hideProgress()
    {
        _progressLabel->hide();
    }
}
