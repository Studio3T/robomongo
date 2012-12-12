#ifndef OUTPUTVIEWER_H
#define OUTPUTVIEWER_H

#include <QWidget>
#include <QWebView>
#include <QSplitter>
#include <editors/PlainJavaScriptEditor.h>
#include "BsonWidget.h"
#include <domain/MongoShellResult.h>

namespace Robomongo
{
    class OutputViewer : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputViewer(QWidget *parent = 0);

        void doSomething(const QList<MongoShellResult> &documents);
        void toggleOrientation();

    private:
        QSplitter *_splitter;
        QWebView *_view;

        RoboScintilla *_configureLogText();
        BsonWidget *_configureBsonWidget();

    };
}

#endif // OUTPUTVIEWER_H
