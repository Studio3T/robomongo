#pragma once

#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/editors/jsedit.h"

namespace Robomongo
{
    class RoboScintilla : public QsciScintilla
    {
    public:
        typedef QsciScintilla BaseClass;
        RoboScintilla(QWidget *parent = NULL);
        void setIgnoreEnterKey(bool ignore) { _ignoreEnterKey = ignore; }
        void setIgnoreTabKey(bool ignore) { _ignoreTabKey = ignore; }

    protected:
        void wheelEvent(QWheelEvent *e);
        void keyPressEvent(QKeyEvent *e);

    private:
        bool _ignoreEnterKey;
        bool _ignoreTabKey;
    };
}
