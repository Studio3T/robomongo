#ifndef PLAINJAVASCRIPTEDITOR_H
#define PLAINJAVASCRIPTEDITOR_H

#include "jsedit.h"

namespace Robomongo
{
    class PlainJavaScriptEditor : public JSEdit
    {
    public:
        PlainJavaScriptEditor(QWidget *parent=NULL);
    private:
        Q_DISABLE_COPY(PlainJavaScriptEditor)
    };
}

#endif // PLAINJAVASCRIPTEDITOR_H
