#include "robomongo/gui/editors/JSLexer.h"

#include <Qsci/qscilexerjavascript.h>

namespace Robomongo
{
    JSLexer::JSLexer(QObject *parent) : QsciLexerJavaScript(parent)
    {
    }

    QColor JSLexer::defaultPaper(int style) const
    {
        return QColor(73, 76, 78);
        //return QColor(48, 10, 36); // Ubuntu-style background
    }

    QColor JSLexer::defaultColor(int style) const
    {
        switch (style)
        {
        case Default:
            return QColor("#FFFFFF");

        case Comment:
        case CommentLine:
            return QColor("#999999");

        case CommentDoc:
        case CommentLineDoc:
            return QColor("#999999");

        case Number:
            //return QColor("#DBF76C");
            return QColor("#FFA09E");

        case Keyword:
            //return QColor("#FDE15D");
            return QColor("#BEE5FF");

        case DoubleQuotedString:
        case SingleQuotedString:
        case RawString:
            //return QColor("#5ED363");
            return QColor("#C6F079");

        case PreProcessor:
            return QColor("#00FF00");

        case Operator:
        case UnclosedString:
            //return QColor("#FF7729");
            //return QColor("#AFBED4");
            return QColor("#FFD14D");


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

    const char *JSLexer::keywords(int set) const
    {
        if (set == 1)
            return
                "abstract boolean break byte case catch char class const continue "
                "debugger default delete do double else enum export extends final "
                "finally float for function goto if implements import in instanceof "
                "int interface long native new package private protected public "
                "return short static super switch synchronized this throw throws "
                "transient try typeof var void volatile while with "
                "ISODate ObjectId Mongo Date NumberInt Number NumberLong Timestamp _id null false true "
                "UUID LUUID PYUUID CSUUID JUUID NUUID ";

        return 0;
    }
}
