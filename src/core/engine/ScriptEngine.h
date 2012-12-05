#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>

namespace Robomongo
{
    class ScriptEngine : public QObject
    {
        Q_OBJECT
    public:
        explicit ScriptEngine();

        void init();


    };
}

#endif // SCRIPTENGINE_H
