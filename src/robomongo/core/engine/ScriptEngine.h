#pragma once

#include <QMutex>
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

        void init(bool isLoadMongoJs);
        MongoShellExecResult exec(const std::string &script, const std::string &dbName = std::string());
        void interrupt();

        void use(const std::string &dbName);
        void setBatchSize(int batchSize);
        void ping();
        QStringList complete(const std::string &prefix);


    private:
        ConnectionSettings *_connection;

        MongoShellResult prepareResult(const std::string &type, const std::string &output, const std::vector<MongoDocumentPtr> &objects, qint64 elapsedms);
        MongoShellExecResult prepareExecResult(const QList<MongoShellResult> &results);

        std::string getString(const char *fieldName);

        bool statementize(const std::string &script, std::vector<std::string> &outList, std::string &outError);
        std::vector<std::string> statementize2(const std::string &script);
        void parseTree(JSParseNode * root, int indent, const std::string &script, std::vector<std::string> &list, bool topList);

        // Order is important. mongo::Scope should be destroyed before mongo::ScriptEngine
        mongo::ScriptEngine *_engine;
        mongo::Scope *_scope;

        std::string subb(const std::vector<std::string> &list, int fline, int fpos, int tline, int tpos) const;

        QMutex _mutex;
    };
}
