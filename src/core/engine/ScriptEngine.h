#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include "boost/scoped_ptr.hpp"
#include "mongo/scripting/engine.h"

namespace Robomongo
{
    class ScriptEngine : public QObject
    {
        Q_OBJECT
    public:
        ScriptEngine(const QString &host, int port, const QString &username, const QString &password, const QString &database);

        void init();
        void exec(const QString &script);

    private:
        QString _host;
        QString _database;
        QString _username;
        QString _password;
        int _port;

        boost::scoped_ptr<mongo::Scope> _scope;

    };
}

#endif // SCRIPTENGINE_H
