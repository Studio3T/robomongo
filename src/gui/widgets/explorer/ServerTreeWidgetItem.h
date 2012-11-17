#ifndef SERVERTREEWIDGETITEM_H
#define SERVERTREEWIDGETITEM_H

#include <QObject>

class MongoServerOld;

class ServerTreeWidgetItem : public QObject, public QTreeWidgetItem
{
	Q_OBJECT

public:
	ServerTreeWidgetItem();
	~ServerTreeWidgetItem();

public slots:
	void refreshDatabases(MongoServerOld * server);
	
private:
	
};

#endif // SERVERTREEWIDGETITEM_H
