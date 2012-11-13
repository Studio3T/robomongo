#ifndef CONNECTIONSDIALOG_H
#define CONNECTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include "ConnectionRecord.h"
#include <boost/scoped_ptr.hpp>

using namespace boost;

// Forward declarations
class ConnectionsDialogViewModel;
class ConnectionDialogViewModel;

/*
** Dialog allows select/edit/add/delete connections
*/
class ConnectionsDialog : public QDialog
{
	Q_OBJECT

private:

	/*
	** Main list widget
	*/
	QListWidget * _listWidget;

	/*
	** View model
	*/
    scoped_ptr<ConnectionsDialogViewModel> _viewModel;

	/*
	** Seems this fields should be deleted from this class 
	*/ 
	ConnectionDialogViewModel * _connection;

public:

	ConnectionsDialog(ConnectionsDialogViewModel * viewModel);

    ConnectionDialogViewModel * connection() { return _connection; }
	virtual void accept();

private slots:
	void edit();
	void add();
	void remove();
	void refresh();
};

#endif // CONNECTIONSDIALOG_H
