#pragma once

#include <QTabWidget>
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

    class OutputWidget : public QTabWidget
    {
        Q_OBJECT

    public:
        explicit OutputWidget(QWidget *parent);

        void present(MongoShell *shell, const std::vector<MongoShellResult> &documents);
        void updatePart(int partIndex, const MongoQueryInfo &queryInfo, 
                        const std::vector<MongoDocumentPtr> &documents);
        void updatePart(int partIndex, const AggrInfo &agrrInfo,
                        const std::vector<MongoDocumentPtr> &documents);
        void toggleOrientation();

        void enterTreeMode();
        void enterTextMode();
        void enterTableMode();
        void enterCustomMode();

        int resultIndex(OutputItemContentWidget *result);

        void showProgress();
        void hideProgress();
        bool progressBarActive() const;

        void applyDockUndockSettings(bool isDocking) const;
        Qt::Orientation getOrientation() const;

    private Q_SLOTS:
        void restoreSize();
        void maximizePart();
        void tabCloseRequested(int);
    private:
        void mouseReleaseEvent(QMouseEvent *event);
        void clearAllParts();
        QString buildStyleSheet();
        void tryToMakeAllPartsEqualInSize();
        std::vector<ViewMode> _prevViewModes;
        int _prevResultsCount;
        QSplitter *_splitter;
        ProgressBarPopup *_progressBarPopup;
        std::vector<OutputItemContentWidget*> _outputItemContentWidgets;
    };
}
