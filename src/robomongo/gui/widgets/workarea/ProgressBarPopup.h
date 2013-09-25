#pragma once
#include <QFrame>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Robomongo
{
    class ProgressBarPopup : public QFrame
    {
        Q_OBJECT

    public:
        ProgressBarPopup(QWidget *parent = NULL);
        enum {heightProgress = 16, widthProgress = 164, height = heightProgress+20, width = widthProgress+20  };
    private:
        QLabel *_progressLabel;
    };
}

