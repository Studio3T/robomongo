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
    class OutputResult;
    class OutputViewer;

    class OutputResultHeader : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputResultHeader(OutputResult *result, OutputWidget *output, QWidget *parent = 0);
        OutputWidget *outputWidget;
        OutputResult *outputResult;

    public slots:
        void showText();
        void showTree();
        void maximizePart();

    private:
        QPushButton *_treeButton;
        QPushButton *_textButton;
        QPushButton *_maxButton;
        bool _maximized;
    };

    class OutputResult : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputResult(OutputViewer *viewer, OutputWidget *output, QWidget *parent = 0);
        OutputWidget *outputWidget;
        OutputViewer *outputViewer;

        OutputResultHeader *header() const { return _header; }
    private:
        OutputResultHeader *_header;
    };

    class OutputViewer : public QWidget
    {
        Q_OBJECT
    public:
        explicit OutputViewer(QWidget *parent = 0);
        ~OutputViewer();

        void doSomething(const QList<MongoShellResult> &documents);
        void toggleOrientation();

        void enterTreeMode();
        void enterTextMode();

        void maximizePart(OutputResult *result);
        void restoreSize();

    private:
        QSplitter *_splitter;
    };
}

#endif // OUTPUTVIEWER_H
