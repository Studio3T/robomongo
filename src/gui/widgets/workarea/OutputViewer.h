#ifndef OUTPUTVIEWER_H
#define OUTPUTVIEWER_H

#include <QWidget>
#include <QSplitter>
#include <editors/PlainJavaScriptEditor.h>
#include "BsonWidget.h"
#include <domain/MongoShellResult.h>
#include "PagingWidget.h"

namespace Robomongo
{
    class OutputWidget;
    class OutputResult;
    class OutputViewer;

    class Indicator : public QWidget
    {
        Q_OBJECT
    public:
        Indicator(const QIcon &icon);
        void setText(const QString &text);
    private:
        QLabel *createLabelWithIcon(const QIcon &icon);
        QLabel *_label;
    };

    class OutputResultHeader : public QFrame
    {
        Q_OBJECT
    public:
        explicit OutputResultHeader(OutputResult *result, OutputWidget *output, QWidget *parent = 0);
        OutputWidget *outputWidget;
        OutputResult *outputResult;
        PagingWidget *paging() const { return _paging; }
        bool treeMode() const { return _treeMode; }

    protected:
        void mouseDoubleClickEvent(QMouseEvent *);

    public slots:
        void showText();
        void showTree();
        void setTime(const QString &time);
        void setCollection(const QString &collection);
        void maximizePart();

    private:
        QLabel *createLabelWithIcon(const QIcon &icon);
        QFrame *createVerticalLine();
        QPushButton *_treeButton;
        QPushButton *_textButton;
        QPushButton *_maxButton;
        Indicator *_collectionIndicator;
        Indicator *_timeIndicator;
        PagingWidget *_paging;
        bool _maximized;
        bool _treeMode;
    };

    class OutputResult : public QFrame
    {
        Q_OBJECT
    public:
        explicit OutputResult(OutputViewer *viewer, OutputWidget *output, const QueryInfo &info, QWidget *parent = 0);
        OutputWidget *outputWidget;
        OutputViewer *outputViewer;

        OutputResultHeader *header() const { return _header; }
        void setQueryInfo(const QueryInfo &queryInfo);
        QueryInfo queryInfo() const { return _queryInfo; }

    private slots:
        void paging_leftClicked(int skip, int limit);
        void paging_rightClicked(int skip, int limit);

    private:
        OutputResultHeader *_header;
        QueryInfo _queryInfo;
    };

    class OutputViewer : public QFrame
    {
        Q_OBJECT
    public:
        explicit OutputViewer(bool textMode, MongoShell *shell, QWidget *parent = 0);
        ~OutputViewer();

        void present(const QList<MongoShellResult> &documents);
        void updatePart(int partIndex, const QueryInfo &queryInfo, const QList<MongoDocumentPtr> &documents);
        void toggleOrientation();

        void enterTreeMode();
        void enterTextMode();

        void maximizePart(OutputResult *result);
        void restoreSize();

        int resultIndex(OutputResult *result);

        MongoShell *shell() const { return _shell; }

    private:
        QSplitter *_splitter;
        bool _textMode;
        MongoShell *_shell;
    };
}

#endif // OUTPUTVIEWER_H
