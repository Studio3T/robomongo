#include "robomongo/core/domain/MongoDocument.h"
#include <QStringBuilder>
#include <QTextCodec>

#include "robomongo/core/domain/MongoDocumentIterator.h"
#include "robomongo/core/domain/MongoElement.h"

using namespace mongo;

namespace Robomongo
{
	Concatenator::Concatenator() : QObject()
	{
		_count = 0;
	}

    void Concatenator::append(const QString &data)
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
        NO_OP;
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
    QList<MongoDocumentPtr> MongoDocument::fromBsonObj(const QList<BSONObj> &bsonObjs)
	{
        QList<MongoDocumentPtr> list;

        foreach(BSONObj bsonObj, bsonObjs){
			list.append(fromBsonObj(bsonObj));
		}

		return list;
	}

	/*
	** Convert to json string
	*/
    void MongoDocument::buildJsonString(Concatenator &con)
	{
		MongoDocumentIterator i(this);


        con.append("{ \n");

		while (i.hasMore())
		{
            MongoElementPtr e = i.next();

            con.append("\"");
            con.append(e->fieldName());
            con.append("\"");
            con.append(" : ");
			e->buildJsonString(con);
            con.append(", \n");
		}

        con.append("\n}\n\n");
	}

	/*
	** Build JsonString from list of documents
	*/
    QString MongoDocument::buildJsonString(const QList<MongoDocumentPtr> &documents)
	{
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        //qt4 QTextCodec::setCodecForCStrings(codec);

        mongo::StringBuilder sb;

		int position = 0;
        foreach(MongoDocumentPtr doc, documents)
		{
			if (position == 0)
                sb << "/* 0 */\n";
			else 
                sb << "\n\n/* " << position << " */\n";

			string jsonString = doc->bsonObj().jsonString(TenGen, 1);

            sb << jsonString;
			position++;
		}

        string final = sb.str();
        QString qJsonString = QString::fromStdString(final);

        return qJsonString;
    }

    QString MongoDocument::buildJsonString(const MongoDocumentPtr &doc)
    {
        QTextCodec *codec = QTextCodec::codecForName("UTF-8");
        //qt4 QTextCodec::setCodecForCStrings(codec);
        string jsonString = doc->bsonObj().jsonString(TenGen, 1);
        QString qJsonString = QString::fromStdString(jsonString);
        return qJsonString;
    }



}
