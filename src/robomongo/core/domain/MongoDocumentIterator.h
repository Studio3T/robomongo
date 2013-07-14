#pragma once
#include <QObject>
#include <mongo/bson/bsonobjiterator.h>
#include "robomongo/core/Core.h"

namespace Robomongo
{
	class MongoDocumentIterator
	{
	public:
		/*
		**
		*/
        MongoDocumentIterator(MongoDocument *const document);

		bool hasMore();

        MongoElementPtr next();

    private:
        MongoDocument *const _document;
        mongo::BSONObjIterator _iterator;
	};
}
