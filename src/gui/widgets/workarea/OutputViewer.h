#ifndef OUTPUTVIEWER_H
#define OUTPUTVIEWER_H

#include <QWidget>
#include <QSplitter>
#include <editors/PlainJavaScriptEditor.h>
#include "BsonWidget.h"
#include <domain/MongoShellResult.h>

namespace Robomongo
{
    class OutputResultHeader : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputResultHeader(QWidget *parent = 0);
    };

    class OutputResult : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputResult(QWidget *contentHeader, QWidget *parent = 0);
    };

    class OutputViewer : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputViewer(QWidget *parent = 0);

        void doSomething(const QList<MongoShellResult> &documents);
        void toggleOrientation();

    private:
        QSplitter *_splitter;

        RoboScintilla *_configureLogText();
        BsonWidget *_configureBsonWidget();

    };
}

#endif // OUTPUTVIEWER_H
