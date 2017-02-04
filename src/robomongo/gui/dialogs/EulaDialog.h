#pragma once

#include <QDialog>

namespace Robomongo
{

    class EulaDialog : public QDialog
    {
        Q_OBJECT

    public:
        static const QSize minimumSize;

        explicit EulaDialog(QWidget *parent = nullptr);
        
    public Q_SLOTS:
        void accept() override;
        void reject() override;

    protected:
        /**
        * @brief Reimplementing closeEvent in order to do some pre-close actions.
        */
        void closeEvent(QCloseEvent *event) override;

    private:

        /**
        * @brief Restore window settings from system registry
        */
        void restoreWindowSettings();

        /**
        * @brief Save window settings into system registry
        */
        void saveWindowSettings() const;

    };
}

