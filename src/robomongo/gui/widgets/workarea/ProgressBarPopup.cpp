#include "ProgressBarPopup.h"

#include <QMovie>
#include <QVBoxLayout>

namespace Robomongo
{

    ProgressBarPopup::ProgressBarPopup(QWidget *parent) : QFrame(parent)
    {
        int w = 163 + 1; // height of progress image
        int h = 15 + 1;  // width of progress image
        int lr = 10;     // padding on left and right
        int tb = 10;      // padding on top and bottom

        setStyleSheet("QFrame {background-color: #e1e1e1; border: 0px solid #c7c5c4; border-radius: 6px;}");

        QMovie *movie = new QMovie(":robomongo/icons/progress_bar.gif", QByteArray(), this);
        _progressLabel = new QLabel();
        _progressLabel->setMovie(movie);
        _progressLabel->setFixedWidth(w);
        _progressLabel->setFixedHeight(h);
        movie->start();

        setFixedSize(w + 2 * lr, h + 2 * tb);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->setContentsMargins(lr, tb, lr, tb);
        layout->setSpacing(0);
        layout->addWidget(_progressLabel);
        setLayout(layout);
    }
}
