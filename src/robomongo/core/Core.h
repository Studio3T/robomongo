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

    class KeyboardManager;
    typedef boost::scoped_ptr<KeyboardManager> KeyboardManagerScopedPtr;

    class MongoServer;
//    typedef boost::shared_ptr<MongoServer> MongoServerPtr;
//    typedef boost::weak_ptr<MongoServer> MongoServerWeakPtr;

    class MongoShell;
//    typedef boost::shared_ptr<MongoShell> MongoShellPtr;

    class MongoDatabase;
//    typedef boost::shared_ptr<MongoDatabase> MongoDatabasePtr;

    class MongoCollection;
    typedef boost::shared_ptr<MongoCollection> MongoCollectionPtr;

    class MongoDocument;
    typedef boost::shared_ptr<MongoDocument> MongoDocumentPtr;

    class MongoElement;
    typedef boost::shared_ptr<MongoElement> MongoElementPtr;

    class ConnectionSettings;
    //typedef QSharedPointer<ConnectionSettings> ConnectionSettingsPtr;

    typedef boost::scoped_ptr<mongo::DBClientConnection> DBClientConnectionScopedPtr;
}

/**
 * @brief NO_OP macro does nothing, but it can be put in places where you need
 * debugger hit, but don't have actual code. Even with NO_OP, you still need to
 * place breakpoint on it.
 */
inline void __dummy_function_for_NO_OP () {}
#define NO_OP __dummy_function_for_NO_OP ()