#pragma once

#include <vector>
#include <QObject>
#include <mongo/bson/bsonobj.h>
#include <mongo/bson/bsonelement.h>

namespace Robomongo
{
    /**
     * @brief BSON tree item (represents array or object)
     */
    struct BsonItemFields
    {
        QString _key;
        QString _value;
        mongo::BSONType _type;
        mongo::BinDataType _binType;
    };

    class BsonTreeItem : public QObject
    {
        Q_OBJECT
    public:
        enum eColumn
        {
            eKey = 0,
            eValue = 1,
            eType = 2,
            eCountColumns = 3
        };

        typedef QObject BaseClass;
        typedef std::vector<BsonTreeItem*> ChildContainerType;

        explicit BsonTreeItem(QObject *parent = 0);
        explicit BsonTreeItem(const mongo::BSONObj &bsonObjRoot, QObject *parent = 0);

        unsigned childrenCount() const;
        void clear();
        void addChild(BsonTreeItem *item);
        void removeChild(BsonTreeItem *item);
        BsonTreeItem* child(unsigned pos) const;
        BsonTreeItem* childSafe(unsigned pos) const;
        BsonTreeItem* childByKey(const QString &val);
        int indexOf(BsonTreeItem *item) const;

        const BsonTreeItem* superParent() const;
        mongo::BSONObj root() const;
        mongo::BSONObj superRoot() const;

        std::string fieldName() const { return _fieldName; };
        void setFieldName(const std::string &fieldName) { _fieldName = fieldName; };

        QString key() const;
        void setKey(const QString &key);

        QString value() const;
        void setValue(const QString &value);

        mongo::BSONType type() const;
        void setType(mongo::BSONType type);

        mongo::BinDataType binType() const;
        void setBinType(mongo::BinDataType type);

    protected:

        const mongo::BSONObj _root;
        ChildContainerType _items;
        BsonItemFields _fields;
        std::string _fieldName;
    };
}
