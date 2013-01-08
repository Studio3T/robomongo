#include <QModelIndex>

#include "robomongo/gui/examples/CredentialModel.h"

using namespace Robomongo;

int CredentialModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return _credentials.count() + 1;
}

int CredentialModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 3;
}

QVariant CredentialModel::data(const QModelIndex &index, int role) const
{
    if ((role == Qt::EditRole || role == Qt::DisplayRole) && index.row() == _credentials.count())
         return QVariant ();

    if (index.column() == 2 && role == Qt::DisplayRole) {
            return "******";
    }

    if (index.isValid() && role == Qt::DisplayRole || role == Qt::EditRole) {
        CredentialSettings *credential = _credentials.value(index.row());
        switch(index.column()) {
            case 0 : return credential->databaseName();
            case 1 : return credential->userName();
            case 2 : return credential->userPassword();
        }
    }


    return QVariant();
}

QVariant CredentialModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section) {
            case 0: return "Database";
            case 1: return "User Name";
            case 2: return "Password";
        }

        return QVariant();
    }

    if (orientation == Qt::Vertical && role == Qt::DisplayRole)
    {
        if (section == _credentials.count())
            return "*";
        else
            return QString::number(section);
    }

    return QVariant();
}

bool CredentialModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {

        CredentialSettings *credential;
        if (index.row() == _credentials.count()) {
            if  (!value.toString().isEmpty()) {
                credential = new CredentialSettings(); //value.toString(), "", "");
                _credentials.append(credential);
            } else
                return false;
        } else {
            credential = _credentials.value(index.row());
        }
        switch(index.column()) {
            case 0 : credential->setDatabaseName(value.toString()); break;
            case 1 : credential->setUserName(value.toString()); break;
            case 2 : credential->setUserPassword(value.toString()); break;
        }

        emit dataChanged(createIndex(0, 0), createIndex(_credentials.count() + 10, 2));
        emit headerDataChanged(Qt::Vertical, 0, _credentials.count() + 10);
        return true;
    }

    return false;
}

Qt::ItemFlags CredentialModel::flags(const QModelIndex &index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsEnabled;
}

void CredentialModel::updateAll()
{
    emit dataChanged(createIndex(0, 0), createIndex(_credentials.count() + 10, 2));
    emit headerDataChanged(Qt::Vertical, 0, _credentials.count() + 10);
}

void CredentialModel::remove(int at)
{
    _credentials.removeAt(at);
}
