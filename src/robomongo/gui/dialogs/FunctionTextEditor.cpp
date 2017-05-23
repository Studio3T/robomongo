#include "robomongo/gui/dialogs/FunctionTextEditor.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <Qsci/qscilexerjavascript.h>
#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo
{
    FunctionTextEditor::FunctionTextEditor(const QString &server, const QString &database,
                                           const MongoFunction &function, QWidget *parent) :
        QDialog(parent),
        _function(function)
    {
        setMinimumWidth(700);
        setMinimumHeight(550);

        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), database);
        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), server);

        _nameEdit = new QLineEdit(QtUtils::toQString(function.name()));

        _queryText = new FindFrame(this);
        _configureQueryText();
        _queryText->sciScintilla()->setText(QtUtils::toQString(_function.code()));

        QFrame *hline = new QFrame(this);
        hline->setFrameShape(QFrame::HLine);
        hline->setFrameShadow(QFrame::Sunken);

        QHBoxLayout *hlayout = new QHBoxLayout();
        hlayout->setContentsMargins(2, 0, 5, 1);
        hlayout->setSpacing(0);
        hlayout->addWidget(serverIndicator, 0, Qt::AlignLeft);
        hlayout->addWidget(databaseIndicator, 0, Qt::AlignLeft);
        hlayout->addStretch(1);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *bottomlayout = new QHBoxLayout();
        bottomlayout->addStretch(1);
        bottomlayout->addWidget(buttonBox);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->addLayout(hlayout);
        layout->addWidget(hline);
        layout->addWidget(new QLabel("Name:"));
        layout->addWidget(_nameEdit);
        layout->addWidget(new QLabel("Code:"));
        layout->addWidget(_queryText);
        layout->addLayout(bottomlayout);
        setLayout(layout);

        _nameEdit->setFocus();
    }

    void FunctionTextEditor::setCursorPosition(int line, int column)
    {
        _queryText->sciScintilla()->setCursorPosition(line, column);
    }

    void FunctionTextEditor::setCode(const QString &code)
    {
        _queryText->sciScintilla()->setText(code);
    }

    void FunctionTextEditor::accept()
    {
        if (_nameEdit->text().isEmpty() && _queryText->sciScintilla()->text().isEmpty())
            return;

        _function.setName(QtUtils::toStdString(_nameEdit->text()));
        _function.setCode(QtUtils::toStdString(_queryText->sciScintilla()->text()));

        BaseClass::accept();
    }

    /*
    ** Configure QsciScintilla query widget
    */
    void FunctionTextEditor::_configureQueryText()
    {
        const QFont &textFont = GuiRegistry::instance().font();
        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        javaScriptLexer->setFont(textFont);        
        _queryText->sciScintilla()->setAppropriateBraceMatching();
        _queryText->sciScintilla()->setFont(textFont);
        _queryText->sciScintilla()->setPaper(QColor(255, 0, 0, 127));
        _queryText->sciScintilla()->setLexer(javaScriptLexer);
        _queryText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_WORD);
        _queryText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        _queryText->sciScintilla()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        _queryText->sciScintilla()->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    }
}
