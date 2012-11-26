#include "MongoDocument.h"
#include "MongoDocumentIterator.h"
#include "MongoElement.h"
#include <QStringBuilder>
#include <QTextCodec>

namespace Robomongo
{
	Concatenator::Concatenator() : QObject()
	{
		_count = 0;
	}

	void Concatenator::append(const QString & data)
	{
		_list.append(data);
		_count = _count + data.length();
	}

	QString Concatenator::build()
	{
		QString text;
		text.reserve(_count + 10);

		foreach(QString s, _list)
		{
			text = text % s;
		}

		return text;
	}




	MongoDocument::MongoDocument() : QObject()
	{
		// test
	}

	MongoDocument::~MongoDocument()
	{

	}

	/*
	** Create MongoDocument from BsonObj. It will take owned version of BSONObj
	*/
	MongoDocument::MongoDocument(BSONObj bsonObj)
	{
		_bsonObj = bsonObj/*.getOwned()*/;
	}

	/*
	** Create MongoDocument from BsonObj. It will take owned version of BSONObj
	*/ 
    MongoDocumentPtr MongoDocument::fromBsonObj(BSONObj bsonObj)
	{
		MongoDocument * doc = new MongoDocument(bsonObj);
        return MongoDocumentPtr(doc);
	}

	/*
	** Create list of MongoDocuments from QList<BsonObj>. It will take owned version of BSONObj
	*/ 
    QList<MongoDocumentPtr> MongoDocument::fromBsonObj(QList<BSONObj> bsonObjs)
	{
        QList<MongoDocumentPtr> list;

		foreach(BSONObj bsonObj, bsonObjs)
		{
			list.append(fromBsonObj(bsonObj));
		}

		return list;
	}

	/*
	** Convert to json string
	*/
	void MongoDocument::buildJsonString(Concatenator * con)
	{
		MongoDocumentIterator i(this);


		con->append("{ \n");

		while (i.hasMore())
		{
			MongoElement * e = i.next();

			con->append("\"");
			con->append(e->fieldName());
			con->append("\"");
			con->append(" : ");
			e->buildJsonString(con);
			con->append(", \n");
		}

		con->append("\n}\n\n");
	}

	/*
	** Build JsonString from list of documents
	*/
    QString MongoDocument::buildJsonString(QList<MongoDocumentPtr> documents)
	{
		Concatenator * con = new Concatenator();

		int position = 0;
        foreach(MongoDocumentPtr doc, documents)
		{
			if (position == 0)
				con->append("/* 0 */\n");
			else 
				con->append(QString("\n\n/* %1 */\n").arg(position));

			string jsonString = doc->bsonObj().jsonString(TenGen, 1);

 			QTextCodec * codec = QTextCodec::codecForName("UTF-8");
 			QTextCodec::setCodecForCStrings(codec);

			QString qJsonString = QString::fromStdString(jsonString);
			con->append(qJsonString);

			position++;
		}

		return con->build();
	}



}
