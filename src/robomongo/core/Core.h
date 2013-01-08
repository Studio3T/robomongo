#ifndef CORE_H
#define CORE_H

#include <QMetaType>
#include <QSharedPointer>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "robomongo/core/Wrapper.h"

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

    typedef boost::scoped_ptr<mongo::DBClientConnection> DBClientConnection_ScopedPtr;
}

/**
 * @brief NO_OP macro does nothing, but it can be put in places where you need
 * debugger hit, but don't have actual code. Even with NO_OP, you still need to
 * place breakpoint on it.
 */
inline void __dummy_function_for_NO_OP () {}
#define NO_OP __dummy_function_for_NO_OP ()

#endif // CORE_H
