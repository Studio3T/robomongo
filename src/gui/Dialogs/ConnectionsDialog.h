#ifndef CONNECTIONSDIALOG_H
#define CONNECTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include "settings/ConnectionRecord.h"
#include <boost/scoped_ptr.hpp>

using namespace boost;

namespace Robomongo
{
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

        QList<ConnectionRecord> _connections;

    public:

        ConnectionsDialog(QList<ConnectionRecord> & connections);

        virtual void accept();

    private slots:
        void edit();
        void add();
        void remove();
        void refresh();
    };
}


#endif // CONNECTIONSDIALOG_H
