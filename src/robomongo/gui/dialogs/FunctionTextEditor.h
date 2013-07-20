#pragma once

#include <QDialog>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

#include "robomongo/core/domain/MongoFunction.h"

namespace Robomongo
{
    class FindFrame;

    class FunctionTextEditor : public QDialog
    {
        Q_OBJECT

    public:
        typedef QDialog BaseClass;
        explicit FunctionTextEditor(const QString &server, const QString &database,
                                    const MongoFunction &code, QWidget *parent = 0);

        MongoFunction function() const { return _function; }

        void setCursorPosition(int line, int column);
        void setCode(const QString &code);

    public Q_SLOTS:
        virtual void accept();

    private:
        void _configureQueryText();
        QLineEdit *_nameEdit;
        FindFrame *_queryText;
        MongoFunction _function;
    };
}

