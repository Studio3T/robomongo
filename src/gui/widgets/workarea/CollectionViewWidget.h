#ifndef COLLECTIONVIEWWIDGET_H
#define COLLECTIONVIEWWIDGET_H

#include <QObject>
#include "mongodb\db\json.h"
#include <Qsci/qsciscintilla.h>
//#include <mongodb/shell/msvc/MongoShell.h>

using namespace mongo;

class JsonTreeWidget;
class QTabWidget;

class CollectionViewWidget : public QWidget
{
	Q_OBJECT

public:
	CollectionViewWidget();
	~CollectionViewWidget();

	void displayObjects(QList<BSONObj> objs);
	void setQuery(QString query);
	void populateObject(QTreeWidgetItem * item, BSONObj obj, bool inArray = false);

	bool eventFilter(QObject * o, QEvent * e);

public slots:
	void jsonItemExpanded(QTreeWidgetItem * item);
	void tabPageChanged(int index);
//	void updateText(const QString & text);
	void updateText(const QString & text);
	void queryLinesCountChanged();
	void execute();

private:
	JsonTreeWidget * _jsonTree;
	QsciScintilla *_jsonText;
	QsciScintilla *_queryText;
	QsciScintilla *_logText;
	QsciScintilla *_resultText;
	QTabWidget * _modeTabs;
	QList<BSONObj> _bsonObjects;
//	MongoShell * _shell;
};

class TextPrepareThread : public QThread 
{
	Q_OBJECT
public:
	TextPrepareThread(QList<BSONObj> bsonObjects)
	{
		_bsonObjects = bsonObjects;
	}

	void run()
	{
		QString text;
		QStringList sl;
		int size = 0;

		foreach(BSONObj obj, _bsonObjects)
		{
			string jsonSt = obj.jsonString(TenGen, 1);
			QString st = QString::fromStdString(jsonSt);
			// 			text = text % st;
			sl.append(st);
			size += st.count();
		}


		text.reserve(size);

		foreach(QString s, sl)
		{
			text = text % s;
		}

		emit finished(text);
//		exec();

		//		exec();
	}

signals:
	void finished(const QString & text); 

private:
	QList<BSONObj> _bsonObjects;
};

#endif // COLLECTIONVIEWWIDGET_H
