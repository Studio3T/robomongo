#ifndef CREDENTIALMODEL_H
#define CREDENTIALMODEL_H

#include <settings/CredentialSettings.h>
#include <QAbstractTableModel>

namespace Robomongo
{
    class CredentialModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:

        CredentialModel(QList<CredentialSettings *> credentials) : _credentials(credentials) {}

        int rowCount(const QModelIndex &parent) const;
        int columnCount(const QModelIndex &parent) const;
        QVariant data(const QModelIndex &index, int role) const;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role);
        Qt::ItemFlags flags(const QModelIndex &index) const;

        void updateAll();
        void remove(int at);

    private:
        QList<CredentialSettings *> _credentials;
    };
}

#endif // CREDENTIALMODEL_H
