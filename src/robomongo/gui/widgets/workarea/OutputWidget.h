#pragma once

#include <QFrame>
QT_BEGIN_NAMESPACE
class QSplitter;
QT_END_NAMESPACE

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/Enums.h"

namespace Robomongo
{
    class OutputItemContentWidget;
    class ProgressBarPopup;
    class MongoShell;

    class OutputWidget : public QFrame
    {
        Q_OBJECT

    public:
        explicit OutputWidget(QWidget *parent);

        void present(MongoShell *shell, const std::vector<MongoShellResult> &documents);
        void updatePart(int partIndex, const MongoQueryInfo &queryInfo, const std::vector<MongoDocumentPtr> &documents);
        void toggleOrientation();

        void enterTreeMode();
        void enterTextMode();
        void enterTableMode();
        void enterCustomMode();

        int resultIndex(OutputItemContentWidget *result);

        void showProgress();
        void hideProgress();

        void applyDockUndockSettings(bool isDocking) const;
        Qt::Orientation getOrientation() const;

    private Q_SLOTS:
        void restoreSize();
        void maximizePart();
    private:
        void clearAllParts();
        std::vector<ViewMode> _prevViewModes;
        int _prevResultsCount;
        void tryToMakeAllPartsEqualInSize();
        QSplitter *_splitter;
        ProgressBarPopup *_progressBarPopup;
        std::vector<OutputItemContentWidget*> _outputItemContentWidgets;
    };
}
