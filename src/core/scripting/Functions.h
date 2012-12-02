#ifndef ROBOFUNCTIONS_H
#define ROBOFUNCTIONS_H

#include <QMetaType>
#include <QScriptValue>

class QScriptContext;
class QScriptEngine;

namespace Robomongo
{
    class ScriptMongoClass
    {
    public:
        ScriptMongoClass();
        ~ScriptMongoClass();
        ScriptMongoClass(const QString &host);
        QString host;
    };

    QScriptValue Mongo_ctor(QScriptContext *context, QScriptEngine *engine);
}

Q_DECLARE_METATYPE(Robomongo::ScriptMongoClass)
Q_DECLARE_METATYPE(Robomongo::ScriptMongoClass*)


#endif // ROBOFUNCTIONS_H
