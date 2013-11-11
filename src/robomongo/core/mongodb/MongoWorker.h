#pragma once

#include <QObject>
#include <QMutex>

#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/settings/ConnectionSettings.h"

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

namespace Robomongo
{
    class ScriptEngine;

    class MongoWorker : public QObject
    {
        Q_OBJECT

    public:
        typedef std::vector<std::string> DatabasesContainerType;
        typedef QObject BaseClass;
        explicit MongoWorker(IConnectionSettingsBase *connection, bool isLoadMongoRcJs, int batchSize, QObject *parent = NULL);

        IConnectionSettingsBase *connectionRecord() const {return _connection;}
        ~MongoWorker();
        enum{ pingTimeMs = 60*1000 };
        virtual void customEvent(QEvent *);      

    protected Q_SLOTS: // handlers:
        /**
         * @brief Initiate connection to MongoDB
         */
        void handle(EstablishConnectionRequest *event);

        /**
         * @brief Inserts document
         */
        void handle(InsertDocumentRequest *event);

        /**
         * @brief Remove documents
         */
        void handle(RemoveDocumentRequest *event);

        /**
         * @brief Load list of all collection names
         */
        void handle(ExecuteQueryRequest *event);

        /**
         * @brief Execute javascript
         */
        void handle(ExecuteScriptRequest *event);

        void handle(AutocompleteRequest *event);

         /**
         * @brief Load list of all database names
         */
        void handle(LoadDatabaseNamesRequest *event);
        void handle(CreateDatabaseRequest *event);
        void handle(DropDatabaseRequest *event);

    private Q_SLOTS:

         void init();

         /**
         * @brief Every minute we are issuing { ping : 1 } command to every used connection
         * in order to avoid dropped connections.
         */
        void keepAlive(ErrorInfo &er);

    protected:
        virtual void timerEvent(QTimerEvent *);

    private:
        void saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns, bool overwrite, ErrorInfo &er); //nothrow
        void removeDocuments(const MongoNamespace &ns, mongo::Query query, bool justOne, ErrorInfo &er); //nothrow
        void dropCollection(const MongoNamespace &ns, ErrorInfo &er); //nothrow
        void duplicateCollection(const MongoNamespace &ns, const std::string &newCollectionName, ErrorInfo &er); //nothrow
        void copyCollectionToDiffServer(MongoWorker *cl, const MongoNamespace &from, const MongoNamespace &to, ErrorInfo &er); //nothrow
        void createCollection(const MongoNamespace &ns, ErrorInfo &er); //nothrow
        void dropDatabase(const std::string &dbName, ErrorInfo &er); //nothrow
        void createDatabase(const std::string &dbName, ErrorInfo &er); //nothrow
        std::vector<std::string> getDatabaseNames(ErrorInfo &er); //nothrow
        std::vector<std::string> getCollectionNames(const std::string &dbname, ErrorInfo &er); //nothrow
        std::vector<MongoCollectionInfo> getCollectionInfos(const std::string &dbname, ErrorInfo &er); //nothrow
        void renameCollection(const MongoNamespace &ns, const std::string &newCollectionName, ErrorInfo &er); //nothrow
        std::vector<MongoDocumentPtr> MongoWorker::query(const MongoQueryInfo &info, ErrorInfo &er); //nothrow
        void dropIndexFromCollection(const MongoCollectionInfo &collection, const std::string &indexName, ErrorInfo &er); //nothrow
        void renameIndexFromCollection(const MongoCollectionInfo &collection, const std::string &oldIndexName, const std::string &newIndexName, ErrorInfo &er); //nothrow
        void ensureIndex(const EnsureIndex &oldInfo,const EnsureIndex &newInfo, ErrorInfo &er); //nothrow
        std::vector<EnsureIndex> getIndexes(const MongoCollectionInfo &collection, ErrorInfo &er); //nothrow
        std::vector<MongoFunction> getFunctions(const std::string &dbName, ErrorInfo &er); //nothrow
        void createFunction(const std::string &dbName, const MongoFunction &fun, const std::string &existingFunctionName, ErrorInfo &er); //nothrow
        void dropFunction(const std::string &dbName, const std::string &name, ErrorInfo &er); //nothrow
        std::vector<MongoUser> getUsers(const std::string &dbName, ErrorInfo &er); //nothrow
        void createUser(const std::string &dbName, const MongoUser &user, bool overwrite, ErrorInfo &er); //nothrow
        void dropUser(const std::string &dbName, const mongo::OID &id, ErrorInfo &er); //nothrow
        float getVersion(ErrorInfo &er);

        std::string getAuthBase() const;        
        mongo::DBClientBase *getConnection(ErrorInfo &er);
        /**
         * @brief Send reply event to object 'obj'
         */
        void reply(QObject *receiver, Event *event);

        mongo::DBClientBase *_dbclient;
        bool _isConnected;
        
        QThread *_thread;
        QMutex _firstConnectionMutex;

        ScriptEngine *_scriptEngine;

        bool _isAdmin;
        const bool _isLoadMongoRcJs;
        const int _batchSize;
        int _timerId;

        IConnectionSettingsBase *_connection;
    };    
}
