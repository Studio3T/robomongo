#pragma once

#include <QWidget>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <mongo/client/dbclient.h>

#include "robomongo/core/domain/MongoFunction.h"

namespace Robomongo
{
    class RoboScintilla;

    class FunctionTextEditor : public QDialog
    {
        Q_OBJECT

    public:
        explicit FunctionTextEditor(const QString &server, const QString &database,
                                    const MongoFunction &code, QWidget *parent = 0);

        MongoFunction function() const { return _function; }

        void setCursorPosition(int line, int column);
        void setCode(const QString &code);

    public slots:
        virtual void accept();

    private:
        void _configureQueryText();
        QFont chooseTextFont();
        QLineEdit *_nameEdit;
        RoboScintilla *_queryText;
        QFont _textFont;
        MongoFunction _function;
    };
}

