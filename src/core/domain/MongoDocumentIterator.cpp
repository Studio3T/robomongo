#include "MongoDocument.h"
#include "MongoElement.h"
#include "MongoDocumentIterator.h"
#include "mongo/client/dbclient.h"

namespace Robomongo
{
	/*
	**
	*/
	MongoDocumentIterator::MongoDocumentIterator(MongoDocument * document) : QObject(), _iterator(document->bsonObj())
	{
		_document = document;
	}

	bool MongoDocumentIterator::hasMore()
	{
		return _iterator.more();
	}

	MongoElement * MongoDocumentIterator::next()
	{
		BSONElement bsonElement = _iterator.next();
		return new MongoElement(bsonElement);
	}
}

