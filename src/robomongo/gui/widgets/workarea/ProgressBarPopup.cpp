#include "ProgressBarPopup.h"

#include <QLabel>
#include <QMovie>
#include <QVBoxLayout>

namespace Robomongo
{

    ProgressBarPopup::ProgressBarPopup(QWidget *parent) :
        QFrame(parent)
    {
        setStyleSheet("QFrame {background-color: #e1e1e1; border: 0px solid #c7c5c4; border-radius: 6px;}");

        QMovie *movie = new QMovie(":robomongo/icons/progress_bar.gif", QByteArray(), this);
        _progressLabel = new QLabel();
        _progressLabel->setMovie(movie);
        _progressLabel->setFixedWidth(widthProgress);
        _progressLabel->setFixedHeight(heightProgress);
        movie->start();

        setFixedSize(width, height);

        QVBoxLayout *layout = new QVBoxLayout();
        layout->setContentsMargins((width-widthProgress)/2, (height-heightProgress)/2, (height-heightProgress)/2, (width-widthProgress)/2);
        layout->setSpacing(0);
        layout->addWidget(_progressLabel);
        setLayout(layout);
    }
}
