#pragma once

#include <QObject>
#include <mongo/client/dbclient.h>

#include "robomongo/core/Core.h"
#include "robomongo/core/domain/MongoElement.h"

namespace Robomongo
{
	class MongoDocumentIterator : public QObject
	{
		Q_OBJECT

	public:
		/*
		**
		*/
        MongoDocumentIterator(MongoDocument *document);

		bool hasMore();

        MongoElementPtr next();

    private:
        MongoDocument *_document;
        mongo::BSONObjIterator _iterator;
	};
}
