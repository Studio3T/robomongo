#include <QStringBuilder>
#include <QStringList>
#include <mongo/client/dbclient.h>

#include "robomongo/core/mongodb/MongoService.h"

using namespace mongo;

MongoService::MongoService(QObject *parent)
	: QObject(parent)
{

}

MongoService::~MongoService()
{

}

DBClientConnection *MongoService::connect(QString address, QString userName, QString password)
{
    DBClientConnection *connection = new DBClientConnection;
	connection->connect(address.toStdString());

//	string errmsg;
//	connection->createPasswordDigest(userName.toStdString(), password.toStdString());
//	bool res = connection->auth(QString("admin").toStdString(), userName.toStdString(), password.toStdString(), errmsg);

	return connection;
}

QStringList MongoService::getDatabases(DBClientConnection *connection)
{
	QStringList stringList;

	list<string> dbs = connection->getDatabaseNames();

	for ( list<string>::iterator i = dbs.begin(); i != dbs.end(); i++ ) {
		stringList.append(QString::fromStdString(*i));
	}

	return stringList;
}

QStringList MongoService::getCollections(DBClientConnection *connection, QString database)
{
	QStringList stringList;

	list<string> dbs = connection->getCollectionNames(database.toStdString());

	for (list<string>::iterator i = dbs.begin(); i != dbs.end(); i++) {
		stringList.append(QString::fromStdString(*i));
	}

	return stringList;
}

QList<BSONObj> MongoService::getAllObjects( DBClientConnection *connection, QString collection )
{
	QList<BSONObj> bsonList;
	auto_ptr<DBClientCursor> cursor = connection->query(collection.toStdString(), BSONObj(), 100);

	while (cursor->more()) 
	{
		BSONObj bsonObj = cursor->next();
		bsonList.append(bsonObj.getOwned());
	}

	return bsonList;	
}

QString MongoService::getStringValue(BSONElement &element)
{
	switch(element.type())
	{
		case NumberLong:
			return QString::number(element.Long());
		case NumberDouble:
			return QString::number(element.Double());
		case NumberInt:
			return QString::number(element.Int());
		case mongo::String:
		{
			/*
			** If you'll write:
			** 
			**   int valsize    = element.valuesize();
			**   int strsize    = element.valuestrsize();
			**   int bytescount = qstrlen(element.valuestr());
			**  
			** You'll get:
			**
			**   bytescount + 1 == strsize
			**   strsize + 4    == valsize
			**
			** So:
			**   bytescount + 5 == valsize
			**
			*/
			return QString::fromUtf8(element.valuestr(), element.valuestrsize() - 1);
		}
		case mongo::Bool:
			return element.Bool() ? "true" : "false";
		case mongo::Date:
			return QString::fromStdString(element.Date().toString());
		case jstOID:
			return QString::fromStdString(element.OID().toString());
		default:
			return "<unsupported>";
	}
}

QString MongoService::toJsonString(QList<BSONObj> bsonObjects)
{
	QString str;

	foreach (BSONObj obj, bsonObjects)
	{
		MongoService::toJsonString(str, obj);
	}

	return str;
}

void MongoService::toJsonString(QString buff, BSONObj bsonObject)
{
	BSONObjIterator i(bsonObject);

	buff = buff % "{ \n";

	while (i.more())
	{
		BSONElement e = i.next();

		if (e.isSimpleType())
		{
			QString fieldName = QString::fromUtf8(e.fieldName());
			QString fieldValue = MongoService::getStringValue(e);

			buff = buff % "\"";
			buff = buff % fieldName;
			buff = buff % "\"";
			buff = buff % " : ";
			buff = buff % fieldValue % ", \n";
		} 
		else if (e.isABSONObj())
		{
			QString fieldName = QString("%1").arg(QString::fromUtf8(e.fieldName()));
			buff = buff % "\"";
			buff = buff % fieldName;
			buff = buff % "\"";
			buff = buff % " : ";
			MongoService::toJsonString(buff, e.Obj());
		}
	}

	buff = buff % "\n}";
}
