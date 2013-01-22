#pragma once

#include <QLabel>

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

