#include "robomongo/core/domain/MongoDocumentIterator.h"

#include "robomongo/core/domain/MongoDocument.h"
#include "robomongo/core/domain/MongoElement.h"

namespace Robomongo
{
    MongoDocumentIterator::MongoDocumentIterator(MongoDocument *const document) :
        _document(document),_iterator(document->bsonObj())
    {

    }

    bool MongoDocumentIterator::hasMore()
    {
        return _iterator.more();
    }

    MongoElementPtr MongoDocumentIterator::next()
    {
        mongo::BSONElement bsonElement = _iterator.next();
        return MongoElementPtr(new MongoElement(bsonElement));
    }
}

