#include "PlainJavaScriptEditor.h"
#include "jsedit.h"

using namespace Robomongo;

PlainJavaScriptEditor::PlainJavaScriptEditor(QWidget *parent) : JSEdit(parent)
{
    JSEdit *editor= this;

    // dark color scheme
    editor->setColor(JSEdit::Background,    QColor("#272822"));
    editor->setColor(JSEdit::Normal,        QColor("#FFFFFF"));
    editor->setColor(JSEdit::Comment,       QColor("#666666"));
    editor->setColor(JSEdit::Number,        QColor("#DBF76C"));
    editor->setColor(JSEdit::String,        QColor("#5ED363"));
    editor->setColor(JSEdit::Operator,      QColor("#FF7729"));
    editor->setColor(JSEdit::Identifier,    QColor("#FFFFFF"));
    editor->setColor(JSEdit::Keyword,       QColor("#FDE15D"));
    editor->setColor(JSEdit::BuiltIn,       QColor("#9CB6D4"));
    editor->setColor(JSEdit::Cursor,        QColor("#272822"));
    editor->setColor(JSEdit::Marker,        QColor("#DBF76C"));
    editor->setColor(JSEdit::BracketMatch,  QColor("#1AB0A6"));
    editor->setColor(JSEdit::BracketError,  QColor("#A82224"));
    editor->setColor(JSEdit::FoldIndicator, QColor("#555555"));

    QStringList keywords = editor->keywords();
    keywords << "db";
    keywords << "find";
    keywords << "limit";
    keywords << "forEach";
    editor->setKeywords(keywords);
}
