#include "robomongo/gui/dialogs/DocumentTextEditor.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <Qsci/qscilexerjavascript.h>
#include <mongo/client/dbclient.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/shell/db/json.h"


namespace Robomongo
{
    DocumentTextEditor::DocumentTextEditor(const QString &server, const QString &database, const QString &collection,
                                           const QString &json, bool readonly /* = false */, QWidget *parent) :
        QDialog(parent),
        _readonly(readonly)
    {
        setMinimumWidth(700);
        setMinimumHeight(550);

        Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), collection);
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);
        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), server);

        QPushButton *validate = new QPushButton("Validate");
        validate->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
        connect(validate, SIGNAL(clicked()), this, SLOT(onValidateButtonClicked()));

        _queryText = new FindFrame(this);
        _configureQueryText();
        _queryText->sciScintilla()->setText(json);
        connect(_queryText->sciScintilla(), SIGNAL(textChanged()), this, SLOT(onQueryTextChanged()));

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->setContentsMargins(2, 0, 5, 1);
        hlayout->setSpacing(0);
        hlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
        hlayout->addWidget(collectionIndicator, 0, Qt::AlignLeft);
        hlayout->addStretch(1);

        QDialogButtonBox *buttonBox = new QDialogButtonBox (this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        QHBoxLayout *bottomlayout = new QHBoxLayout();
        bottomlayout->addWidget(validate);
        bottomlayout->addStretch(1);
        bottomlayout->addWidget(buttonBox);

        QVBoxLayout *layout = new QVBoxLayout();

        // show top bar only if we have info for it
        if (!(server.isEmpty() && database.isEmpty() && collection.isEmpty()))
            layout->addLayout(hlayout);

        layout->addWidget(_queryText);
        layout->addLayout(bottomlayout);
        setLayout(layout);

        if (_readonly)
            buttonBox->button(QDialogButtonBox::Save)->hide();
    }

    QString DocumentTextEditor::jsonText() const
    {
        return _queryText->sciScintilla()->text();
    }

    void DocumentTextEditor::setCursorPosition(int line, int column)
    {
        _queryText->sciScintilla()->setCursorPosition(line, column);
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
        try {
            _obj = mongo::Robomongo::fromjson(QtUtils::toStdString<std::string>(text));
        } catch (mongo::ParseMsgAssertionException &ex) {
            QString message = QString::fromStdString(ex.reason());
            int offset = ex.offset();

            int line, pos;
            _queryText->sciScintilla()->lineIndexFromPosition(offset, &line, &pos);
            _queryText->sciScintilla()->setCursorPosition(line, pos);

            int lineHeight = _queryText->sciScintilla()->lineLength(line);
            _queryText->sciScintilla()->fillIndicatorRange(line, pos, line, lineHeight, 0);

            message = QString("Unable to parse JSON:<br /> <b>%1</b>, at (%2, %3).")
                .arg(message).arg(line + 1).arg(pos + 1);

            QMessageBox::critical(NULL, "Parsing error", message);
            _queryText->setFocus();
            activateWindow();
            return false;
        }

        if (!silentOnSuccess) {
            QMessageBox::information(NULL, "Validation", "JSON is valid!");
            _queryText->setFocus();
            activateWindow();
        }

        return true;
    }

    void DocumentTextEditor::onQueryTextChanged()
    {
        _queryText->sciScintilla()->clearIndicatorRange(0, 0, _queryText->sciScintilla()->lines(), 40, 0);
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
        QFont font = chooseTextFont();
        javaScriptLexer->setFont(font);
        _queryText->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
        _queryText->sciScintilla()->setFont(font);
        _queryText->sciScintilla()->setPaper(QColor(255, 0, 0, 127));
        _queryText->sciScintilla()->setLexer(javaScriptLexer);
        _queryText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_WORD);
        _queryText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        _queryText->sciScintilla()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        _queryText->sciScintilla()->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    }

    QFont DocumentTextEditor::chooseTextFont() const
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
}
