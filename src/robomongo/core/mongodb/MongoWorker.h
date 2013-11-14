#pragma once

#include <QObject>
#include <QMutex>

#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/events/MongoEventsInfo.hpp"

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
        void saveDocument(EventsInfo::SaveDocumentInfo &inf); //nothrow
        void removeDocuments(EventsInfo::RemoveDocumentInfo &inf); //nothrow
        void dropCollection(EventsInfo::DropCollectionInfo &inf); //nothrow
        void duplicateCollection(EventsInfo::DuplicateCollectionInfo &inf); //nothrow
        void copyCollectionToDiffServer(EventsInfo::CopyCollectionToDiffServerInfo &inf); //nothrow
        void createCollection(EventsInfo::CreateCollectionInfo &inf); //nothrow
        void dropDatabase(EventsInfo::DropDatabaseInfo &inf); //nothrow
        void createDatabase(EventsInfo::CreateDataBaseInfo &inf); //nothrow
        void establishConnection(EventsInfo::EstablishConnectionInfo &inf); //nothrow
        void getCollectionInfos(EventsInfo::LoadCollectionInfo &inf); //nothrow
        void renameCollection(EventsInfo::RenameCollectionInfo &inf); //nothrow
        void query(EventsInfo::ExecuteQueryInfo &inf); //nothrow
        void dropIndexFromCollection(EventsInfo::DeleteIndexInfo &inf); //nothrow        
        void ensureIndex(EventsInfo::CreateIndexInfo &inf); //nothrow
        void getIndexes(EventsInfo::LoadCollectionIndexesInfo &inf); //nothrow
        void getFunctions(EventsInfo::LoadFunctionInfo &inf); //nothrow
        void createFunction(EventsInfo::CreateFunctionInfo &inf); //nothrow
        void dropFunction(EventsInfo::DropFunctionInfo &inf); //nothrow
        void getUsers(EventsInfo::LoadUserInfo &inf); //nothrow
        void createUser(EventsInfo::CreateUserInfo &inf); //nothrow
        void dropUser(EventsInfo::DropUserInfo &inf); //nothrow

        std::vector<std::string> getDatabaseNames(ErrorInfo &er); //nothrow
        std::vector<std::string> getCollectionNames(const std::string &dbname, ErrorInfo &er); //nothrow  
        
        float getVersion(ErrorInfo &er); //nothrow

        std::string getAuthBase() const;        
        mongo::DBClientBase *getConnection(ErrorInfo &er);

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
