#ifndef CONNECTIONSDIALOG_H
#define CONNECTIONSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include <QHash>
#include "settings/ConnectionRecord.h"
#include "settings/SettingsManager.h"
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
        QListWidget *_listWidget;
        SettingsManager *_settingsManager;

        QHash<ConnectionRecord, QListWidgetItem *> _hash;

    public:

        ConnectionsDialog(SettingsManager * manager);

        virtual void accept();

    private slots:
        void add(ConnectionRecord);
        void update(ConnectionRecord);
        void remove(ConnectionRecord);
        void edit();
        void add();
        void remove();
        void refresh();

        void set(QListWidgetItem *item, ConnectionRecord record);
    };
}


#endif // CONNECTIONSDIALOG_H
