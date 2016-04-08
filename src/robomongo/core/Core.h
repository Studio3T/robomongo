#pragma once

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

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
    typedef boost::scoped_ptr<AppRegistry> AppRegistryScopedPtr;

    class SettingsManager;
    typedef boost::scoped_ptr<SettingsManager> SettingsManagerScopedPtr;

    class App;
    typedef boost::scoped_ptr<App> AppScopedPtr;

    class EventBus;
    typedef boost::scoped_ptr<EventBus> EventBusScopedPtr;

    class MongoCollection;
    typedef boost::shared_ptr<MongoCollection> MongoCollectionPtr;

    class MongoDocument;
    typedef boost::shared_ptr<MongoDocument> MongoDocumentPtr;

    enum ConnectionType {
        // This type of connection is shown in Explorer and also
        // opens SSH tunnel for Secondary connections (if needed)
        ConnectionPrimary      = 0,

        // Never shown in Explorer and uses SSH tunnel from
        // Primary connection (if needed)
        ConnectionSecondary    = 1,

        // The same as Primary, but is specifically for testing
        // connections.
        ConnectionTest         = 2
    };
}
