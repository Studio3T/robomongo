#ifndef JSLEXER_H
#define JSLEXER_H

#include <QObject>
#include <QColor>
#include "Qsci/qscilexerjavascript.h"

namespace Robomongo
{
    class JSLexer : public QsciLexerJavaScript
    {
        Q_OBJECT

    public:
        JSLexer(QObject *parent = 0);
        QColor defaultPaper(int style) const;
        QColor defaultColor(int style) const;
    };
}

#endif // JSLEXER_H
