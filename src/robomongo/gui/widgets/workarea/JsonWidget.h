#ifndef JSONWIDGET_H
#define JSONWIDGET_H

#include <QWidget>
#include <mongodb\client\dbclient.h>
#include <QListWidgetItem>
#include <Qsci\qsciscintilla.h>

using namespace mongo;

/*
** Widget displays JSON/BSON object in two modes - tree mode and text mode.
** In text mode you can edit bson.
*/
class JsonWidget : public QWidget
{
	Q_OBJECT

public:

	/*
	** 
	*/
	JsonWidget(BSONObj bsonObject, QWidget *parent);

	/*
	** 
	*/
	~JsonWidget();

	/*
	** Get and set custom data for this widget
	*/
	void setCustomData(void * data) { _customData = data; }
	void * customData() const { return _customData; }

	/*
	** Populate tree with BSONObject 
	** If item is null we are adding top level item.
	*/
	void populateObject(QTreeWidgetItem * item, BSONObj obj, bool inArray);

	/*
	** Collapse widget
	** In collapsed mode only one line of widget is visible. 
	*/
	void collapse();

	/*
	** Expand widget
	** In expanded mode widget is fully available
	*/
	void expand();

public slots:

	/*
	** Toggle between expanded and collapsed mode
	*/
	void toggle();

	/*
	** Fires when tab page is changed(from Tree mode to Text mode)
	*/
	void tabPageChanged(int index);

	/*
	** Fires when tree item expanded by user
	*/
	void treeItemExpanded(QTreeWidgetItem * item);

	/*
	** Fires when tree item collapsed by user
	*/
	void treeItemCollapsed(QTreeWidgetItem * item);

signals:
	/*
	** Geometry of widget is changed
	*/
	void geometryChanged(JsonWidget * jsonWidget, QSize & size);

private:

	void * _customData;

	/*
	** BSON object that we are displaying
	*/
	BSONObj _bsonObject;

	/*
	** Expand/collapse button
	*/
	QPushButton * _toggleButton;

	/*
	** Tree widget for tree mode
	*/
	QTreeWidget * _treeWidget;

	/*
	** Displays bson object as JSON text
	*/
	QsciScintilla * _textEdit;

	/*
	** Allows to switch between tree mode and text mode
	*/
	QTabWidget * _modeTabWidget;

	/*
	** Was this widget initialized (i.e. was it already in expanded state)
	*/
	bool _initialized;

	/*
	** Height of widget before collapsing
	*/
	int _height;

	/*
	** Height of widget in collapsed mode;
	*/
	static const int COLLAPSED_HEIGHT = 33;

	/*
	** Create and configure tree widget 
	*/
	void createTreeWidget();

	/*
	** Create and configure text edit 
	*/
	void createTextEdit();

	/*
	** Create and configure mode tab widget
	*/
	void createModeTabWidget();

	/*
	** Create and configure toggle button
	*/
	void createToggleButton();

	/*
	** Create and configure layout of widget
	*/
	void createLayout();

	/*
	** Resize to content
	*/
	void resizeToContent();

	int getNumberOfExpandedItems(QTreeWidgetItem * item);
};

#endif // JSONWIDGET_H
