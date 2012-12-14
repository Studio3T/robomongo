#include "JSLexer.h"
#include "Qsci/qscilexerjavascript.h"

using namespace Robomongo;

JSLexer::JSLexer(QObject *parent) : QsciLexerJavaScript(parent)
{
}

QColor JSLexer::defaultPaper(int style) const
{
    return QColor(48, 10, 36);
}

QColor JSLexer::defaultColor(int style) const
{
    switch (style)
    {
    case Default:
        return QColor("#FFFFFF");

    case Comment:
    case CommentLine:
        return QColor("#666666");

    case CommentDoc:
    case CommentLineDoc:
        return QColor("#666666");

    case Number:
        return QColor("#DBF76C");

    case Keyword:
        return QColor("#FDE15D");

    case DoubleQuotedString:
    case SingleQuotedString:
    case RawString:
        return QColor("#5ED363");

    case PreProcessor:
        return QColor("#FFFFFF");

    case Operator:
    case UnclosedString:
        return QColor("#FF7729");

    case Regex:
        return QColor("#FFFFFF");

    case CommentDocKeyword:
        return QColor("#FFFFFF");

    case CommentDocKeywordError:
        return QColor("#FFFFFF");

    case InactiveDefault:
    case InactiveUUID:
    case InactiveCommentLineDoc:
    case InactiveKeywordSet2:
    case InactiveCommentDocKeyword:
    case InactiveCommentDocKeywordError:
        return QColor("#FFFFFF");

    case InactiveComment:
    case InactiveCommentLine:
    case InactiveNumber:
        return QColor("#FFFFFF");

    case InactiveCommentDoc:
        return QColor("#FFFFFF");

    case InactiveKeyword:
        return QColor("#FFFFFF");

    case InactiveDoubleQuotedString:
    case InactiveSingleQuotedString:
    case InactiveRawString:
        return QColor("#FFFFFF");

    case InactivePreProcessor:
        return QColor("#FFFFFF");

    case InactiveOperator:
    case InactiveIdentifier:
    case InactiveGlobalClass:
        return QColor("#FFFFFF");

    case InactiveUnclosedString:
        return QColor("#FFFFFF");

    case InactiveVerbatimString:
        return QColor("#FFFFFF");

    case InactiveRegex:
        return QColor("#FFFFFF");
    }

    return QColor("#FFFFFF");
//    return QsciLexer::defaultColor(style);
}
