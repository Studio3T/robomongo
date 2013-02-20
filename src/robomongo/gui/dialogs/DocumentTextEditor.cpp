#include "robomongo/gui/dialogs/DocumentTextEditor.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <Qsci/qscilexerjavascript.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/shell/db/json.h"


using namespace Robomongo;

DocumentTextEditor::DocumentTextEditor(const QString &server, const QString &database, const QString &collection,
                                       const QString &json, QWidget *parent) :
    QDialog(parent)
{
    setMinimumWidth(600);
    setMinimumHeight(450);

    Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), collection);
    Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);
    Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), server);

    QPushButton *cancel = new QPushButton("Cancel");
    connect(cancel, SIGNAL(clicked()), this, SLOT(close()));

    QPushButton *save = new QPushButton("Save");
    connect(save, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *validate = new QPushButton("Validate");
    connect(validate, SIGNAL(clicked()), this, SLOT(onValidateButtonClicked()));

    _queryText = new RoboScintilla;
    _textFont = chooseTextFont();
    _configureQueryText();
    _queryText->setText(json);
    connect(_queryText, SIGNAL(textChanged()), this, SLOT(onQueryTextChanged()));

    QHBoxLayout *hlayout = new QHBoxLayout();
    hlayout->setContentsMargins(2, 0, 5, 1);
    hlayout->setSpacing(0);
    hlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
    hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
    hlayout->addWidget(collectionIndicator, 0, Qt::AlignLeft);
    hlayout->addStretch(1);

    QHBoxLayout *bottomlayout = new QHBoxLayout();
    bottomlayout->addWidget(validate);
    bottomlayout->addStretch(1);
    bottomlayout->addWidget(save, 0, Qt::AlignRight);
    bottomlayout->addWidget(cancel, 0, Qt::AlignRight);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addLayout(hlayout);
    layout->addWidget(_queryText);
    layout->addLayout(bottomlayout);
    setLayout(layout);
}

QString DocumentTextEditor::jsonText() const
{
    return _queryText->text();
}

void DocumentTextEditor::setCursorPosition(int line, int column)
{
    _queryText->setCursorPosition(line, column);
}

void DocumentTextEditor::accept()
{
    if (!validate())
        return;

    QDialog::accept();
}

bool DocumentTextEditor::validate(bool silentOnSuccess /* = true */)
{
    QString text = jsonText();
    QByteArray utf = text.toUtf8();
    try {
        _obj = mongo::Robomongo::fromjson(utf.data());
    } catch (mongo::ParseMsgAssertionException &ex) {
        QString message = QString::fromStdString(ex.reason());
        int offset = ex.offset();

        int line, pos;
        _queryText->lineIndexFromPosition(offset, &line, &pos);
        _queryText->setCursorPosition(line, pos);

        int lineHeight = _queryText->lineLength(line);
        _queryText->fillIndicatorRange(line, pos, line, lineHeight, 0);

        message = QString("Unable to parse JSON:<br /> <b>%1</b>, at (%2, %3).")
            .arg(message).arg(line + 1).arg(pos + 1);

        QMessageBox::critical(NULL, "Parsing error", message);
        _queryText->setFocus();
        activateWindow();
        return false;
    }

    if (!silentOnSuccess) {
        QMessageBox::information(NULL, "Validation", "JSON is valid!");
    }

    return true;
}

void DocumentTextEditor::onQueryTextChanged()
{
    _queryText->clearIndicatorRange(0, 0, _queryText->lines(), 40, 0);
}

void DocumentTextEditor::onValidateButtonClicked()
{
    validate(false);
}

/*
** Configure QsciScintilla query widget
*/
void DocumentTextEditor::_configureQueryText()
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
    _queryText->setMatchedBraceBackgroundColor(QColor(48, 10, 36));
    _queryText->setMatchedBraceForegroundColor(QColor("#1AB0A6"));
    _queryText->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);
    _queryText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _queryText->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    _queryText->setStyleSheet("QFrame { background-color: rgb(48, 10, 36); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
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
