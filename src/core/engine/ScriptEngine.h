#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

#include <QObject>
#include "boost/scoped_ptr.hpp"
#include "mongo/scripting/engine.h"
#include "js/jsparse.h"
#include "Result.h"
#include <QMutex>

namespace Robomongo
{
    class ScriptEngine : public QObject
    {
        Q_OBJECT
    public:
        ScriptEngine(const QString &host, int port, const QString &username, const QString &password, const QString &database);
        ~ScriptEngine();

        void init();
        QList<Result> exec(const QString &script, const QString &dbName = QString());

        void use(const QString &dbName);

    private:
        QString _host;
        QString _database;
        QString _username;
        QString _password;
        bool statementize(const QString &script, QStringList &outList, QString &outError);
        QStringList statementize2(const QString &script);
        void parseTree(JSParseNode * root, int indent, const QString &script, QStringList &list, bool topList);
        int _port;

        // Order is important. mongo::Scope should be destroyed before mongo::ScriptEngine
        boost::scoped_ptr<mongo::ScriptEngine> _engine;
        boost::scoped_ptr<mongo::Scope> _scope;

        QString subb(const QStringList &list, int fline, int fpos, int tline, int tpos);

        QMutex _mutex;


    };
}

#endif // SCRIPTENGINE_H
