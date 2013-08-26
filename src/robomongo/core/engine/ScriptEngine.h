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
        MongoShellExecResult exec(const std::string &script, const std::string &dbName = std::string());
        void interrupt();

        void use(const std::string &dbName);
        void ping();
        QStringList complete(const QString &prefix);


    private:
        ConnectionSettings *_connection;
        QString _currentDatabase;
        bool _isCurrentDatabaseValid;

        MongoShellResult prepareResult(const std::string &type, const std::string &output, const std::vector<MongoDocumentPtr> &objects, qint64 elapsedms);
        MongoShellExecResult prepareExecResult(const QList<MongoShellResult> &results);

        std::string getString(const char *fieldName);

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
