#pragma once

#include <QObject>

namespace Robomongo
{
    class Wrapper
    {
    public:
        Wrapper(QObject *obj) : _obj(obj) {}
        void invoke(char * methodName, QGenericArgument arg1 = QGenericArgument(), QGenericArgument arg2 = QGenericArgument(),
                                       QGenericArgument arg3 = QGenericArgument(), QGenericArgument arg4 = QGenericArgument(),
                                       QGenericArgument arg5 = QGenericArgument());

    private:
        QObject *_obj;
    };
}
