#include "robomongo/gui/widgets/workarea/PagingWidget.h"

#include <QLabel>
#include <QHBoxLayout>

#include "robomongo/gui/GuiRegistry.h"

namespace Robomongo
{
    PagingWidget::PagingWidget()
    {
        _skipEdit = new QLineEdit;
        _limitEdit = new QLineEdit;
        _skipEdit->setAlignment(Qt::AlignHCenter);
        _skipEdit->setToolTip("Skip");
        _limitEdit->setAlignment(Qt::AlignHCenter);
        _limitEdit->setToolTip("Limit");

        QFontMetrics metrics = _skipEdit->fontMetrics();
        int width = metrics.boundingRect("000000").width();
        _skipEdit->setFixedWidth(width);
        _limitEdit->setFixedWidth(width);

        QPushButton *leftButton = createButtonWithIcon(GuiRegistry::instance().leftIcon());
        QPushButton *rightButton = createButtonWithIcon(GuiRegistry::instance().rightIcon());
        connect(leftButton, SIGNAL(clicked()), this, SLOT(leftButton_clicked()));
        connect(rightButton, SIGNAL(clicked()), this, SLOT(rightButton_clicked()));

        QHBoxLayout *layout = new QHBoxLayout();
        layout->setSpacing(0);
        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(leftButton);
        layout->addSpacing(0);
        layout->addWidget(_skipEdit);
        layout->addSpacing(1);
        layout->addWidget(_limitEdit);
        layout->addSpacing(0);
        layout->addWidget(rightButton);
        setLayout(layout);
    }

    void PagingWidget::setSkip(int skip)
    {
        _skipEdit->setText(QString::number(skip));
        show();
    }

    void PagingWidget::setLimit(int limit)
    {
        if (limit <= 0)
            limit = 50;

        _limitEdit->setText(QString::number(limit));
        show();
    }

    QPushButton *PagingWidget::createButtonWithIcon(const QIcon &icon)
    {
        QPushButton *button = new QPushButton;
        button->setIcon(icon);
        button->setFixedSize(24, 24);
        button->setFlat(true);
        return button;
    }

    void PagingWidget::leftButton_clicked()
    {
        int limit = _limitEdit->text().toInt();
        int skip = _skipEdit->text().toInt();
        emit leftClicked(skip, limit);
    }

    void PagingWidget::rightButton_clicked()
    {
        int limit = _limitEdit->text().toInt();
        int skip = _skipEdit->text().toInt();
        emit rightClicked(skip, limit);
    }
}
