#include "robomongo/core/Wrapper.h"

#include <QMetaObject>

using namespace Robomongo;

void Wrapper::invoke(char *methodName, QGenericArgument arg1, QGenericArgument arg2, QGenericArgument arg3,
                                       QGenericArgument arg4, QGenericArgument arg5)
{
    QMetaObject::invokeMethod(_obj, methodName, Qt::QueuedConnection, arg1, arg2, arg3, arg4, arg5);
}
