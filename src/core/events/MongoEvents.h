#ifndef MONGOEVENTS_H
#define MONGOEVENTS_H

#include <QString>
#include <QStringList>
#include <QEvent>

namespace Robomongo
{
    class CollectionNamesLoaded : public QEvent
    {
    public:
        const static QEvent::Type EventType;
        CollectionNamesLoaded(const QString &databaseName, const QStringList &collectionNames)
            : _databaseName(databaseName), _collectionNames(collectionNames), QEvent(EventType) { }

        QString databaseName() const { return _databaseName; }
        QStringList collectionNames() const { return _collectionNames; }

    private:
        QString _databaseName;
        QStringList _collectionNames;
    };

    class DatabaseNamesLoaded : public QEvent
    {
    public:
        const static QEvent::Type EventType;
        DatabaseNamesLoaded(const QStringList &databaseNames)
            : _databaseNames(databaseNames), QEvent(EventType) { }

        QStringList databaseNames() const { return _databaseNames; }

    private:
        QStringList _databaseNames;
    };

    class ConnectionEstablished : public QEvent
    {
    public:
        const static QEvent::Type EventType;
        ConnectionEstablished(const QString &address)
            : _address(address), QEvent(EventType) { }

        QString address() const { return _address; }

    private:
        QString _address;
    };

    class ConnectionFailed : public QEvent
    {
    public:
        const static QEvent::Type EventType;
        ConnectionFailed(const QString &address)
            : _address(address), QEvent(EventType) { }

        QString address() const { return _address; }

    private:
        QString _address;
    };

}



#endif // MONGOEVENTS_H
