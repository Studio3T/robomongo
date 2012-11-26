#ifndef MONGOSERVICE_H
#define MONGOSERVICE_H

#include <QObject>
#include <mongo/client/dbclient.h>

using namespace mongo;

class MongoService : public QObject
{
	Q_OBJECT

public:
	MongoService(QObject *parent = NULL);
	~MongoService();

	DBClientConnection * connect(QString address, QString userName, QString password);

	QStringList getDatabases(DBClientConnection * connection);
	QStringList getCollections(DBClientConnection * connection, QString database);
	QList<BSONObj> getAllObjects(DBClientConnection * connection, QString collection);

	static QString getStringValue(BSONElement & element);

	static QString toJsonString(QList<BSONObj> bsonObjects);
	static void toJsonString(QString buff, BSONObj bsonObjects);

private:
	
};

#endif // MONGOSERVICE_H
