#include "robomongo/core/domain/MongoServer.h"

#include "robomongo/core/domain/MongoDatabase.h"
#include "robomongo/core/settings/ConnectionSettings.h"
#include "robomongo/core/settings/SshSettings.h"
#include "robomongo/core/settings/SettingsManager.h"
#include "robomongo/core/mongodb/MongoWorker.h"
#include "robomongo/core/mongodb/SshTunnelWorker.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"
#include "robomongo/core/utils/QtUtils.h"

namespace Robomongo {
    R_REGISTER_EVENT(MongoServerLoadingDatabasesEvent)

    MongoServer::MongoServer(ConnectionSettings *settings, bool visible) : QObject(),
        _version(0.0f),
        _visible(visible),
        _client(NULL),
        _sshWorker(NULL),
        _isConnected(false),
        _settings(settings),
        _bus(AppRegistry::instance().bus()) { }

    bool MongoServer::isConnected() const {
        return _isConnected;
    }

    ConnectionSettings *MongoServer::connectionRecord() const {
        return _settings;
    }

    MongoServer::~MongoServer() {
        clearDatabases();

        if (_client != NULL) {
            _client->stopAndDelete();
        }

        // MongoWorker "_client" does not deleted here, because it is now owned by
        // another thread (call to moveToThread() made in MongoWorker constructor).
        // It will be deleted by this thread by means of "deleteLater()", which
        // is also specified in MongoWorker constructor.

        if (_sshWorker != NULL) {
            _sshWorker->stopAndDelete();
        }

        delete _settings;
    }

    /**
     * @brief Try to connect to MongoDB server.
     * @throws MongoException, if fails
     */
    void MongoServer::tryConnect() {
        _bus->send(_client, new EstablishConnectionRequest(this));
    }

    QStringList MongoServer::getDatabasesNames() const {
        QStringList result;
        for (QList<MongoDatabase *>::const_iterator it = _databases.begin(); it != _databases.end(); ++it) {
            MongoDatabase *datab = *it;
            result.append(QtUtils::toQString(datab->name()));
        }
        return result;
    }

    void MongoServer::createDatabase(const std::string &dbName) {
        _bus->send(_client,new CreateDatabaseRequest(this, dbName));
    }

    MongoDatabase *MongoServer::findDatabaseByName(const std::string &dbName) const {
        for (QList<MongoDatabase *>::const_iterator it = _databases.begin(); it != _databases.end(); ++it) {
            MongoDatabase *datab = *it;
            if (datab->name() == dbName) {
                return datab;
            }
        }
        return NULL;
    }

    void MongoServer::dropDatabase(const std::string &dbName) {
        _bus->send(_client,new DropDatabaseRequest(this, dbName));
    }

    void MongoServer::insertDocuments(const std::vector<mongo::BSONObj> &objCont,
                                      const MongoNamespace &ns) {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            insertDocument(*it, ns);
        }
    }

    void MongoServer::insertDocument(const mongo::BSONObj &obj, const MongoNamespace &ns) {
        _bus->send(_client,new InsertDocumentRequest(this, obj, ns));
    }

    void MongoServer::saveDocuments(const std::vector<mongo::BSONObj> &objCont, const MongoNamespace &ns) {
        for (std::vector<mongo::BSONObj>::const_iterator it = objCont.begin(); it != objCont.end(); it++) {
            saveDocument(*it,ns);
        }
    }

    void MongoServer::saveDocument(const mongo::BSONObj &obj, const MongoNamespace &ns) {
        _bus->send(_client, new InsertDocumentRequest(this, obj, ns, true));
    }

    void MongoServer::removeDocuments(mongo::Query query, const MongoNamespace &ns, bool justOne) {
        _bus->send(_client, new RemoveDocumentRequest(this, query, ns, justOne));
    }

    void MongoServer::loadDatabases() {
        _bus->publish(new MongoServerLoadingDatabasesEvent(this));
        _bus->send(_client, new LoadDatabaseNamesRequest(this));
    }

    void MongoServer::clearDatabases() {
        qDeleteAll(_databases);
        _databases.clear();
    }

    void MongoServer::addDatabase(MongoDatabase *database) {
        _databases.append(database);
    }

    void MongoServer::handle(EstablishConnectionResponse *event) {
        if (event->isError()) {
            _isConnected = false;
            _bus->publish(new ConnectionFailedEvent(this, event->error()));
            return;
        }

        const ConnectionInfo &info = event->info();
        _version = info._version;
        _isConnected = true;

        // Do nothing if this is not "visible" connection
        if (!_visible) {
            return;
        }

        _bus->publish(new ConnectionEstablishedEvent(this));
        clearDatabases();
        for(std::vector<std::string>::const_iterator it = info._databases.begin(); it != info._databases.end(); ++it) {
            const std::string &name = *it;
            MongoDatabase *db  = new MongoDatabase(this, name);
            addDatabase(db);
        }
    }

    void MongoServer::handle(LoadDatabaseNamesResponse *event) {
        if (event->isError()) {
            _bus->publish(new ConnectionFailedEvent(this, event->error()));
            return;
        }

        clearDatabases();
        for(std::vector<std::string>::iterator it = event->databaseNames.begin(); it != event->databaseNames.end(); ++it) {
            const std::string &name = *it;
            MongoDatabase *db  = new MongoDatabase(this, name);
            addDatabase(db);
        }

        _bus->publish(new DatabaseListLoadedEvent(this, _databases));
    }

    void MongoServer::handle(InsertDocumentResponse *event) {
        _bus->publish(new InsertDocumentResponse(event->sender(), event->error()));
    }

    void MongoServer::handle(RemoveDocumentResponse *event) {
        _bus->publish(new RemoveDocumentResponse(event->sender(), event->error()));
    }

    void MongoServer::runWorkerThread() {
        _client = new MongoWorker(_settings->clone(),
            AppRegistry::instance().settingsManager()->loadMongoRcJs(),
            AppRegistry::instance().settingsManager()->batchSize());
    }
}
