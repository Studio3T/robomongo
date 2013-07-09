#include "robomongo/gui/dialogs/DocumentTextEditor.h"

#include <QApplication>
#include <QtGui>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
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

        QPushButton *cancel = new QPushButton("Cancel");
        connect(cancel, SIGNAL(clicked()), this, SLOT(close()));

        QPushButton *save = new QPushButton("Save");
        save->setIcon(qApp->style()->standardIcon(QStyle::SP_ArrowRight));
        connect(save, SIGNAL(clicked()), this, SLOT(accept()));

        QPushButton *validate = new QPushButton("Validate");
        validate->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
        connect(validate, SIGNAL(clicked()), this, SLOT(onValidateButtonClicked()));

        _queryText = new FindFrame(this);
        _textFont = chooseTextFont();
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

        QHBoxLayout *bottomlayout = new QHBoxLayout();
        bottomlayout->addWidget(validate);
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

        // show top bar only if we have info for it
        if (!(server.isEmpty() && database.isEmpty() && collection.isEmpty()))
            layout->addLayout(hlayout);

        layout->addWidget(_queryText);
        layout->addLayout(bottomlayout);
        setLayout(layout);

        if (_readonly)
            save->hide();
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
        QByteArray utf = text.toUtf8();
        try {
            _obj = mongo::Robomongo::fromjson(utf.data());
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
        javaScriptLexer->setFont(_textFont);
        _queryText->sciScintilla()->setBraceMatching(QsciScintilla::StrictBraceMatch);
        _queryText->sciScintilla()->setFont(_textFont);
        _queryText->sciScintilla()->setPaper(QColor(255, 0, 0, 127));
        _queryText->sciScintilla()->setLexer(javaScriptLexer);
        _queryText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);
        _queryText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _queryText->sciScintilla()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        // Wrap mode turned off because it introduces huge performance problems
        // even for medium size documents.
        _queryText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_NONE);

        _queryText->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
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
}
