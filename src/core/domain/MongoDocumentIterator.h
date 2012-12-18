#ifndef MONGODOCUMENTITERATOR_H
#define MONGODOCUMENTITERATOR_H

#include <QObject>
#include "Core.h"
#include "MongoElement.h"
#include "mongo/client/dbclient.h"

using namespace mongo;

namespace Robomongo
{
	class MongoDocumentIterator : public QObject
	{
		Q_OBJECT

	private:

        MongoDocument *_document;
		BSONObjIterator _iterator;

	public:
		/*
		**
		*/
        MongoDocumentIterator(MongoDocument *document);

		bool hasMore();

        MongoElementPtr next();
	};
}

#endif // MONGODOCUMENTITERATOR_H
