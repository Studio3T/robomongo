#pragma once

#include <Qsci/qsciscintilla.h>

#include "robomongo/gui/editors/jsedit.h"

namespace Robomongo
{
    class PlainJavaScriptEditor : public JSEdit
    {
    public:
        PlainJavaScriptEditor(QWidget *parent=NULL);

    private:
        Q_DISABLE_COPY(PlainJavaScriptEditor)
    };

    class RoboScintilla : public QsciScintilla
    {
    public:
        RoboScintilla(QWidget *parent = NULL);
        void setIgnoreEnterKey(bool ignore) { _ignoreEnterKey = ignore; }
        void setIgnoreTabKey(bool ignore) { _ignoreTabKey = ignore; }

    protected:
        void paintEvent(QPaintEvent *e);
        void wheelEvent(QWheelEvent *e);
        void keyPressEvent(QKeyEvent *e);

    private:
        bool _ignoreEnterKey;
        bool _ignoreTabKey;
    };
}
