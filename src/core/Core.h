#ifndef CORE_H
#define CORE_H

#include <QMetaType>
#include <QSharedPointer>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "Wrapper.h"

namespace mongo
{
    class DBClientConnection;
}

/*
** Smart pointers for Mongo* staff
*/
namespace Robomongo
{
    class AppRegistry;
    typedef boost::scoped_ptr<AppRegistry> AppRegistry_ScopedPtr;

    class SettingsManager;
    typedef boost::scoped_ptr<SettingsManager> SettingsManager_ScopedPtr;

    class App;
    typedef boost::scoped_ptr<App> AppScopedPtr;

    class Dispatcher;
    typedef boost::scoped_ptr<Dispatcher> DispatcherScopedPtr;

    class MongoServer;
    typedef boost::shared_ptr<MongoServer> MongoServerPtr;
    typedef boost::weak_ptr<MongoServer> MongoServerWeakPtr;

    class MongoShell;
    typedef boost::shared_ptr<MongoShell> MongoShellPtr;

    class MongoDatabase;
    typedef boost::shared_ptr<MongoDatabase> MongoDatabasePtr;

    class MongoCollection;
    typedef boost::shared_ptr<MongoCollection> MongoCollectionPtr;

    class MongoDocument;
    typedef boost::shared_ptr<MongoDocument> MongoDocumentPtr;

    class MongoElement;
    typedef boost::shared_ptr<MongoElement> MongoElementPtr;

    class ConnectionRecord;
    typedef QSharedPointer<ConnectionRecord> ConnectionRecordPtr;

    typedef boost::scoped_ptr<mongo::DBClientConnection> DBClientConnection_ScopedPtr;

}

#endif // CORE_H
