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

    private:
        QLabel *_progressLabel;
    };
}

