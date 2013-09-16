#pragma  once

#include <QDialog>

namespace Robomongo
{
    class AboutDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit AboutDialog(QWidget *parent);
    };
}
