#ifndef OutputWidget_H
#define OutputWidget_H

#include <QWidget>
#include <QSplitter>
#include <editors/PlainJavaScriptEditor.h>
#include "BsonWidget.h"
#include <domain/MongoShellResult.h>
#include "PagingWidget.h"

namespace Robomongo
{
    class OutputItemContentWidget;
    class OutputItemWidget;
    class OutputWidget;

    class OutputWidget : public QFrame
    {
        Q_OBJECT
    public:
        explicit OutputWidget(bool textMode, MongoShell *shell, QWidget *parent = 0);
        ~OutputWidget();

        void present(const QList<MongoShellResult> &documents);
        void updatePart(int partIndex, const QueryInfo &queryInfo, const QList<MongoDocumentPtr> &documents);
        void toggleOrientation();

        void enterTreeMode();
        void enterTextMode();

        void maximizePart(OutputItemWidget *result);
        void restoreSize();

        int resultIndex(OutputItemWidget *result);

        MongoShell *shell() const { return _shell; }

    private:
        QSplitter *_splitter;
        bool _textMode;
        MongoShell *_shell;
    };
}

#endif // OutputWidget_H
