#ifndef OUTPUTVIEWER_H
#define OUTPUTVIEWER_H

#include <QWidget>
#include <QSplitter>
#include <editors/PlainJavaScriptEditor.h>
#include "BsonWidget.h"
#include <domain/MongoShellResult.h>

namespace Robomongo
{
    class OutputWidget;

    class OutputResultHeader : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputResultHeader(OutputWidget *output, QWidget *parent = 0);
        OutputWidget *outputWidget;

    protected slots:
        void showText();
        void showTree();
    };

    class OutputResult : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputResult(OutputWidget *output, QWidget *parent = 0);
        OutputWidget *outputWidget;
    };

    class OutputViewer : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputViewer(QWidget *parent = 0);
        ~OutputViewer();

        void doSomething(const QList<MongoShellResult> &documents);
        void toggleOrientation();

    private:
        QSplitter *_splitter;
    };
}

#endif // OUTPUTVIEWER_H
