#pragma once

#include <QMutex>
#include <mongo/scripting/engine.h>
//#include <third_party/js-1.7/jsparse.h>

#include "robomongo/core/domain/MongoShellResult.h"
#include "robomongo/core/Enums.h"

namespace Robomongo
{
    class ConnectionSettings;

    class ScriptEngine
    {

    public:
        ScriptEngine(ConnectionSettings *connection, int timeoutSec);
        ~ScriptEngine();

        void init(bool isLoadMongoJs);
        MongoShellExecResult exec(const std::string &script, const std::string &dbName = std::string());
        void interrupt();

        void use(const std::string &dbName);
        void setBatchSize(int batchSize);
        void ping();
        QStringList complete(const std::string &prefix, const AutocompletionMode mode);

        void invalidateDbCollectionsCache();

    private:
        ConnectionSettings *_connection;

        MongoShellResult prepareResult(const std::string &type, const std::string &output, const std::vector<MongoDocumentPtr> &objects, qint64 elapsedms);
        MongoShellExecResult prepareExecResult(const std::vector<MongoShellResult> &results);

        std::string getString(const char *fieldName);

        bool statementize(const std::string &script, std::vector<std::string> &outList, std::string &outError);
        std::vector<std::string> statementize2(const std::string &script);
//        void parseTree(JSParseNode * root, int indent, const std::string &script, std::vector<std::string> &list, bool topList);

        int _timeoutSec;
        mongo::ScriptEngine *_engine;
        mongo::Scope *_scope;
        QMutex _mutex;
        bool _initialized;
    };
}
