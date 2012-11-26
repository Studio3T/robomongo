#ifndef MONGODOCUMENT_H
#define MONGODOCUMENT_H

#include <QObject>
#include <QStringList>
#include "mongo/client/dbclient.h"
#include "Core.h"

using namespace mongo;



namespace Robomongo
{
	class Concatenator;

	/*
	** Represents MongoDB object.
	*/
	class MongoDocument : public QObject
	{
		Q_OBJECT

	private:

		/*
		** Owned BSONObj
		*/
		BSONObj _bsonObj;

	public:

		/*
		** Constructs empty Document, i.e. { }
		*/
		MongoDocument();
		~MongoDocument();


		/*
		** Create MongoDocument from BsonObj. It will take owned version of BSONObj
		*/
		MongoDocument(BSONObj bsonObj);

		/*
		** Create MongoDocument from BsonObj. It will take owned version of BSONObj
		*/ 
        static MongoDocumentPtr fromBsonObj(BSONObj bsonObj);

		/*
		** Create list of MongoDocuments from QList<BsonObj>. It will take owned version of BSONObj
		*/ 
        static QList<MongoDocumentPtr> fromBsonObj(QList<BSONObj> bsonObj);
		
		/*
		** Return "native" BSONObj
		*/
		BSONObj bsonObj() const { return _bsonObj; }

		/*
		** Convert to json string
		*/
		void buildJsonString(Concatenator * con);

		/*
		** Build JsonString from list of documents
		*/
        static QString buildJsonString(QList<MongoDocumentPtr> documents);
	};

	class Concatenator : public QObject
	{
		Q_OBJECT

	private:

		QStringList _list;
		int _count;

	public:

		Concatenator();

		void append(const QString & data);

		QString build();
	};
}

#endif // MONGODOCUMENT_H
