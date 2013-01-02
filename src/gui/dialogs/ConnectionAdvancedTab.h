#ifndef CONNECTIONADVANCEDTAB_H
#define CONNECTIONADVANCEDTAB_H

#include <QWidget>
#include <QLineEdit>

namespace Robomongo
{
    class ConnectionAdvancedTab : public QWidget
    {
        Q_OBJECT
    public:
        ConnectionAdvancedTab();
        QLineEdit *_defaultDatabaseName;
    };
}

#endif // CONNECTIONADVANCEDTAB_H
