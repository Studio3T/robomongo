#include "mongo/client/dbclient.h"

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoElement.h"
#include "robomongo/core/domain/MongoDocumentIterator.h"

using namespace mongo;

namespace Robomongo
{
	/*
	**
	*/
    MongoDocumentIterator::MongoDocumentIterator(MongoDocument *document) :
        QObject(),
        _document(document),
        _iterator(document->bsonObj())
	{

	}

	bool MongoDocumentIterator::hasMore()
	{
		return _iterator.more();
	}

    MongoElementPtr MongoDocumentIterator::next()
	{
		BSONElement bsonElement = _iterator.next();
        return MongoElementPtr(new MongoElement(bsonElement));
	}
}

