#pragma once

#include <QWidget>
#include <QLabel>
#include <QSplitter>

#include "robomongo/gui/editors/PlainJavaScriptEditor.h"
#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/gui/widgets/workarea/PagingWidget.h"
#include "robomongo/core/domain/MongoShell.h"

namespace Robomongo
{
    class OutputItemContentWidget;
    class OutputItemWidget;
    class OutputWidget;
    class ProgressBarPopup;

    class OutputWidget : public QFrame
    {
        Q_OBJECT

    public:
        explicit OutputWidget(MongoShell *shell, QWidget *parent = 0);
        ~OutputWidget();

        void present(const std::vector<MongoShellResult> &documents);
        void updatePart(int partIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents);
        void toggleOrientation();

        void enterTreeMode();
        void enterTextMode();
        void enterTableMode();
        void enterCustomMode();

        void maximizePart(OutputItemWidget *result);
        void restoreSize();

        int resultIndex(OutputItemWidget *result);

        void showProgress();
        void hideProgress();

        MongoShell *shell() const { return _shell; }

    private:
        void clearAllParts();
        void tryToMakeAllPartsEqualInSize();
        QSplitter *_splitter;
        ProgressBarPopup *_progressBarPopup;
        MongoShell *_shell;
    };
}
