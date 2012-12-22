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

#define R_EVENT(EVENT_TYPE) \
    else if (_roboevent->type() == EVENT_TYPE::EventType) \
        handle(static_cast<EVENT_TYPE *>(_roboevent));

#define R_HANDLE(EVENT) \
    QEvent *_roboevent = static_cast<QEvent *>((EVENT)); \
    if (false) ;

#define R_MESSAGE \
    public: \
        const static QEvent::Type EventType; \
        const static int nothing; \
        virtual const char *typeString(); \
        virtual const QEvent::Type type();

#define R_REGISTER_EVENT_TYPE(EVENT_TYPE) \
    const QEvent::Type EVENT_TYPE::EventType = static_cast<QEvent::Type>(QEvent::registerEventType()); \
    const char *EVENT_TYPE::typeString() { return #EVENT_TYPE"*"; } \
    const QEvent::Type EVENT_TYPE::type() { return EVENT_TYPE::EventType; } \
    const int EVENT_TYPE::nothing = qRegisterMetaType<EVENT_TYPE*>(#EVENT_TYPE"*");


#endif // CORE_H
