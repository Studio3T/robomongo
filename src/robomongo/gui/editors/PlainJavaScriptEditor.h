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

    protected:
        void paintEvent(QPaintEvent *e);
        void wheelEvent(QWheelEvent *e);
        void keyPressEvent(QKeyEvent *e);
    };
}
