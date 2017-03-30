#include "robomongo/gui/dialogs/DocumentTextEditor.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QDesktopWidget>
#include <QSettings>
#include <Qsci/qscilexerjavascript.h>

#include <mongo/client/dbclientinterface.h>

#include "robomongo/gui/editors/JSLexer.h"
#include "robomongo/gui/editors/FindFrame.h"
#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/gui/widgets/workarea/IndicatorLabel.h"
#include "robomongo/gui/GuiRegistry.h"

#include "robomongo/core/utils/QtUtils.h"
#include "robomongo/shell/bson/json.h"


namespace Robomongo
{
    const QSize DocumentTextEditor::minimumSize = QSize(800, 400);

    DocumentTextEditor::DocumentTextEditor(const CollectionInfo &info, const QString &json, bool readonly /* = false */, QWidget *parent) :
        QDialog(parent),
        _info(info),
        _readonly(readonly)
    {
        QRect screenGeometry = QApplication::desktop()->availableGeometry();
        int horizontalMargin = (int)(screenGeometry.width() * 0.35);
        int verticalMargin = (int)(screenGeometry.height() * 0.20);
        QSize size(screenGeometry.width() - horizontalMargin,
                   screenGeometry.height() - verticalMargin);

        QSettings settings("3T", "Robomongo");
        if (settings.contains("DocumentTextEditor/size"))
        {
            restoreWindowSettings();
        }
        else
        {
            resize(size);
        }

        setWindowFlags(Qt::Window | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);

        Indicator *collectionIndicator = new Indicator(GuiRegistry::instance().collectionIcon(), QtUtils::toQString(_info._ns.collectionName()));
        Indicator *databaseIndicator = new Indicator(GuiRegistry::instance().databaseIcon(), QtUtils::toQString(_info._ns.databaseName()));
        Indicator *serverIndicator = new Indicator(GuiRegistry::instance().serverIcon(), QtUtils::toQString(detail::prepareServerAddress(_info._serverAddress)));

        QPushButton *validate = new QPushButton("Validate");
        validate->setIcon(qApp->style()->standardIcon(QStyle::SP_MessageBoxInformation));
        VERIFY(connect(validate, SIGNAL(clicked()), this, SLOT(onValidateButtonClicked())));

        _queryText = new FindFrame(this);
        _configureQueryText();
        _queryText->sciScintilla()->setText(json);
        // clear modification state after setting the content
        _queryText->sciScintilla()->setModified(false);

        VERIFY(connect(_queryText->sciScintilla(), SIGNAL(textChanged()), this, SLOT(onQueryTextChanged())));

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
        VERIFY(connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept())));
        VERIFY(connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject())));

        QHBoxLayout *bottomlayout = new QHBoxLayout();
        bottomlayout->addWidget(validate);
        bottomlayout->addStretch(1);
        bottomlayout->addWidget(buttonBox);

        QVBoxLayout *layout = new QVBoxLayout();

        // show top bar only if we have info for it
        if (_info.isValid())
            layout->addLayout(hlayout);

        layout->addWidget(_queryText);
        layout->addLayout(bottomlayout);
        setLayout(layout);

        if (_readonly) {
            validate->hide();
            buttonBox->button(QDialogButtonBox::Save)->hide();
            _queryText->sciScintilla()->setReadOnly(true);
        }
    }

    QString DocumentTextEditor::jsonText() const
    {
        return _queryText->sciScintilla()->text().trimmed();
    }

    void DocumentTextEditor::setCursorPosition(int line, int column)
    {
        _queryText->sciScintilla()->setCursorPosition(line, column);
    }

    void DocumentTextEditor::accept()
    {
        if (!validate())
            return;

        saveWindowSettings();

        QDialog::accept();
    }

    void DocumentTextEditor::reject()
    {
        if (_queryText->sciScintilla()->isModified()) {
            int ret = QMessageBox::warning(this, tr("Robomongo"),
                               tr("The document has been modified.\n"
                                  "Do you want to save your changes?"),
                               QMessageBox::Save | QMessageBox::Discard
                               | QMessageBox::Cancel,
                               QMessageBox::Save);

            if (ret == QMessageBox::Save) {
                this->accept();
            } else if (ret == QMessageBox::Discard) {
                QDialog::reject();
            }

            return;
        }

        saveWindowSettings();

        QDialog::reject();
    }

    bool DocumentTextEditor::validate(bool silentOnSuccess /* = true */)
    {
        QString text = jsonText();
        int len = 0;
        try {
            std::string textString = QtUtils::toStdString(text);
            const char *json = textString.c_str();
            int jsonLen = textString.length();
            int offset = 0;
            _obj.clear();
            while (offset != jsonLen)
            {
                mongo::BSONObj doc = mongo::Robomongo::fromjson(json+offset, &len);
                _obj.push_back(doc);
                offset += len;
            }
        } catch (const mongo::Robomongo::ParseMsgAssertionException &ex) {
//            v0.9
            QString message = QtUtils::toQString(ex.reason());
            int offset = ex.offset();

            int line = 0, pos = 0;
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

    void DocumentTextEditor::closeEvent(QCloseEvent *event)
    {
        saveWindowSettings();
        QWidget::closeEvent(event);
    }

    /*
    ** Configure QsciScintilla query widget
    */
    void DocumentTextEditor::_configureQueryText()
    {
        QsciLexerJavaScript *javaScriptLexer = new JSLexer(this);
        QFont font = GuiRegistry::instance().font();
        javaScriptLexer->setFont(font);
        _queryText->sciScintilla()->setAppropriateBraceMatching();
        _queryText->sciScintilla()->setFont(font);
        _queryText->sciScintilla()->setPaper(QColor(255, 0, 0, 127));
        _queryText->sciScintilla()->setLexer(javaScriptLexer);
        _queryText->sciScintilla()->setWrapMode((QsciScintilla::WrapMode)QsciScintilla::SC_WRAP_WORD);
        _queryText->sciScintilla()->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        _queryText->sciScintilla()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        _queryText->sciScintilla()->setStyleSheet("QFrame { background-color: rgb(73, 76, 78); border: 1px solid #c7c5c4; border-radius: 4px; margin: 0px; padding: 0px;}");
    }

    void DocumentTextEditor::saveWindowSettings() const
    {
        QSettings settings("3T", "Robomongo");
        settings.setValue("DocumentTextEditor/size", size());
    }

    void DocumentTextEditor::restoreWindowSettings()
    {
        QSettings settings("3T", "Robomongo");
        resize(settings.value("DocumentTextEditor/size").toSize());
    }

}
