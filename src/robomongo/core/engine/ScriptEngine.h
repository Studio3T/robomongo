#pragma once

#include <QMutex>
#include <boost/scoped_ptr.hpp>
#include <mongo/scripting/engine.h>
#include <js/jsparse.h>

#include "robomongo/core/domain/MongoShellResult.h"

namespace Robomongo
{
    class ConnectionSettings;

    class ScriptEngine
    {

    public:
        ScriptEngine(ConnectionSettings *connection);
        ~ScriptEngine();

        void init();
        MongoShellExecResult exec(const QString &script, const QString &dbName = QString());
        void interrupt();

        void use(const QString &dbName);
        void ping();
        QStringList complete(const QString &prefix);


    private:
        ConnectionSettings *_connection;
        QString _currentDatabase;
        bool _isCurrentDatabaseValid;

        MongoShellResult prepareResult(const QString &type, const QString &output, const QList<MongoDocumentPtr> objects, qint64 elapsedms);
        MongoShellExecResult prepareExecResult(const QList<MongoShellResult> &results);

        QString getString(const char *fieldName);

        bool statementize(const QString &script, QStringList &outList, QString &outError);
        QStringList statementize2(const QString &script);
        void parseTree(JSParseNode * root, int indent, const QString &script, QStringList &list, bool topList);

        // Order is important. mongo::Scope should be destroyed before mongo::ScriptEngine
        mongo::ScriptEngine *_engine;
        mongo::Scope *_scope;

        QString subb(const QStringList &list, int fline, int fpos, int tline, int tpos) const;

        QMutex _mutex;
    };
}
