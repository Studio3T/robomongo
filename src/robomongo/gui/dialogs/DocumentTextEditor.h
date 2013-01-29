#pragma once

#include <QWidget>
#include <QDialog>

namespace Robomongo
{
    class RoboScintilla;

    class DocumentTextEditor : public QDialog
    {
        Q_OBJECT

    public:
        explicit DocumentTextEditor(const QString &server, const QString &database, const QString &collection,
                                    const QString &json, QWidget *parent = 0);
    private:
        void _configureQueryText();
        QFont chooseTextFont();
        RoboScintilla *_queryText;
        QFont _textFont;
    };
}

