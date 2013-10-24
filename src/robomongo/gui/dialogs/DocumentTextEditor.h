#pragma once

#include <QDialog>
#include <mongo/bson/bsonobj.h>
#include "robomongo/core/domain/MongoQueryInfo.h"

namespace Robomongo
{
    class FindFrame;

    class DocumentTextEditor : public QDialog
    {
        Q_OBJECT

    public:
        typedef std::vector<mongo::BSONObj> ReturnType;
        static const QSize minimumSize;

        explicit DocumentTextEditor(const CollectionInfo &info, const QString &json, bool readonly = false, QWidget *parent = 0);

        QString jsonText() const;

        /**
         * @brief Use returned BSONObj only if Dialog exec() method returns QDialog::Accepted
         */
        ReturnType bsonObj() const { return _obj; }

        void setCursorPosition(int line, int column);

    public Q_SLOTS:
        virtual void accept();
        bool validate(bool silentOnSuccess = true);

    private Q_SLOTS:
        void onQueryTextChanged();
        void onValidateButtonClicked();

    private:
        void _configureQueryText();
        const CollectionInfo _info;
        FindFrame *_queryText;
        bool _readonly;
        ReturnType _obj;
    };
}

