#ifndef TESTSTACKPANEL_H
#define TESTSTACKPANEL_H

#include <QWidget>
#include <QAbstractScrollArea>
#include <editors/PlainJavaScriptEditor.h>

namespace Robomongo
{
    class TestStackPanel : public QWidget
    {
        Q_OBJECT
    public:
        explicit TestStackPanel(QWidget *parent = 0);

    private:
        RoboScintilla * _queryText1;
        RoboScintilla * _queryText2;

        void updateSize();
    };

    class TestAbstractAreay : public QAbstractScrollArea
    {

    };

}

#endif // TESTSTACKPANEL_H
