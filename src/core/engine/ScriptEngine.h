#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include "boost/scoped_ptr.hpp"
#include "mongo/scripting/engine.h"
#include "js/jsparse.h"

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
        QStringList statementize(const QString &script);
        void parseTree(JSParseNode * root, int indent, const QString &script, QStringList &list, bool topList);
        int _port;

        boost::scoped_ptr<mongo::Scope> _scope;
        QString subb(const QStringList &list, int fline, int fpos, int tline, int tpos);


    };
}

#endif // SCRIPTENGINE_H
