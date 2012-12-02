#include "Functions.h"
#include <QtScript>

using namespace Robomongo;

QScriptValue Robomongo::Mongo_ctor(QScriptContext *context, QScriptEngine *engine)
{
    QString host = context->argument(0).toString();
    QScriptValue res = engine->toScriptValue(ScriptMongoClass(host));
    res.setProperty("host", host);
    res.setPrototype(context->callee().property("prototype"));
    return res;
}


ScriptMongoClass::ScriptMongoClass()
{
}

ScriptMongoClass::~ScriptMongoClass()
{
}

ScriptMongoClass::ScriptMongoClass(const QString &host) :
    host(host)
{

}
