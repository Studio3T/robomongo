#include "ConnectionRecord.h"

/*
** Constructs connection record
*/
ConnectionRecord::ConnectionRecord() :
    _id(0), _databasePort(27017)
{
}

/*
** Destructs connection record
*/
ConnectionRecord::~ConnectionRecord()
{
}

ConnectionRecord * ConnectionRecord::clone()
{
	ConnectionRecord * connection = new ConnectionRecord;
	connection->setConnectionName(connectionName());
	connection->setDatabaseAddress(databaseAddress());
	connection->setDatabasePort(databasePort());
	connection->setUserName(userName());
	connection->setUserPassword(userPassword());
	return connection;
}
