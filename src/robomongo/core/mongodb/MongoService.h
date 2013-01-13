#pragma once

#include <QObject>
#include <mongo/client/dbclient.h>

class MongoService : public QObject
{
	Q_OBJECT

public:
	MongoService(QObject *parent = NULL);
	~MongoService();

    mongo::DBClientConnection *connect(QString address, QString userName, QString password);

    QStringList getDatabases(mongo::DBClientConnection *connection);
    QStringList getCollections(mongo::DBClientConnection *connection, QString database);
    QList<mongo::BSONObj> getAllObjects(mongo::DBClientConnection *connection, QString collection);

    static QString getStringValue(mongo::BSONElement &element);

    static QString toJsonString(QList<mongo::BSONObj> bsonObjects);
    static void toJsonString(QString buff, mongo::BSONObj bsonObjects);
};
