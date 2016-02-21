#pragma once

#include <QObject>
#include <QColor>
#include <Qsci/qscilexerjavascript.h>

namespace Robomongo
{
    class JSLexer : public QsciLexerJavaScript
    {
        Q_OBJECT
        // This Q_OBJECT macro produce the following error for VC, but works on GCC:
        // unresolved external symbol "public: static struct QMetaObject const...
        // Q_OBJECT

    public:
        JSLexer(QObject *parent = 0);
        QColor defaultPaper(int style) const;
        QColor defaultColor(int style) const;
        const char *keywords(int set) const;
    };
}
