#ifndef CORE_H
#define CORE_H

#include <QSharedPointer>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

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

    class MongoManager;
    typedef boost::scoped_ptr<MongoManager> MongoManagerScopedPtr;

    class MongoServer;
    typedef QSharedPointer<MongoServer> MongoServerPtr;
    typedef QWeakPointer<MongoServer> MongoServerWeakPtr;

    class MongoDatabase;
    typedef QSharedPointer<MongoDatabase> MongoDatabasePtr;

    class ConnectionRecord;
    typedef boost::ptr_vector<ConnectionRecord> ConnectionRecord_PtrVector;
    typedef QSharedPointer<ConnectionRecord> ConnectionRecordPtr;

    typedef boost::scoped_ptr<mongo::DBClientConnection> DBClientConnection_ScopedPtr;

}


#endif // CORE_H
