#include "robomongo/gui/widgets/workarea/BsonTreeItem.h"
#include <mongo/client/dbclientinterface.h>

using namespace mongo;
namespace
{
    struct removeIfFound : public std::unary_function<const Robomongo::BsonTreeItem*, bool> 
    {
        removeIfFound(Robomongo::BsonTreeItem *item) :_whatSearch(item) {}
        bool operator()(const Robomongo::BsonTreeItem* item) const
        {
            if (item == _whatSearch) {
                delete _whatSearch;
                return true;
            }
            return false;
        }
        const Robomongo::BsonTreeItem *const _whatSearch;
    };

    const Robomongo::BsonTreeItem *findSuperRoot(const Robomongo::BsonTreeItem *const item)
    {
        Robomongo::BsonTreeItem *parent = qobject_cast<Robomongo::BsonTreeItem *>(item->parent());
        if (parent) {
            Robomongo::BsonTreeItem *grParent = qobject_cast<Robomongo::BsonTreeItem *>(parent->parent());
            if (grParent) {
               return findSuperRoot(parent);
            }
        }
        return item;
    }
}
namespace Robomongo
{
    BsonTreeItem::BsonTreeItem(QObject *parent) 
        :BaseClass(parent)
    {
       
    }

    BsonTreeItem::BsonTreeItem(const mongo::BSONObj &bsonObjRoot, QObject *parent)
        :BaseClass(parent),
        _root(bsonObjRoot)
    {

    }

    unsigned BsonTreeItem::childrenCount() const
    {
        return _items.size();
    }

    void BsonTreeItem::clear()
    {
        _items.clear();
    }

    void BsonTreeItem::addChild(BsonTreeItem *item)
    {
        _items.push_back(item);
    }

    BsonTreeItem* BsonTreeItem::child(unsigned pos) const
    {
        return _items[pos];
    }

    BsonTreeItem* BsonTreeItem::childSafe(unsigned pos) const
    {
        if (childrenCount() > pos) {
            return _items[pos];
        }
        else {
            return NULL;
        }
    }

    BsonTreeItem* BsonTreeItem::childByKey(const QString &val)
    {
        for (unsigned i = 0; i < _items.size(); ++i) {
            if (_items[i]->key() == val) {
                return _items[i];
            }
        }
        return NULL;
    }

    const BsonTreeItem *BsonTreeItem::superParent() const
    {
        return findSuperRoot(this);
    }

    mongo::BSONObj BsonTreeItem::superRoot() const
    {
        return superParent()->root();
    }

    mongo::BSONObj BsonTreeItem::root() const
    {
        return _root;
    }

    int BsonTreeItem::indexOf(BsonTreeItem *item) const
    {
        for (unsigned i = 0; i < _items.size(); ++i) {
            if (item == _items[i]) {
                return i;
            }
        }
        return -1;
    }

    QString BsonTreeItem::key() const
    {
        return _fields._key;
    }

    QString BsonTreeItem::value() const
    {
        return _fields._value;
    }

    mongo::BSONType BsonTreeItem::type() const
    {
        return _fields._type;
    }

    void BsonTreeItem::setKey(const QString &key)
    {
        _fields._key = key;
    }

    void BsonTreeItem::setValue(const QString &value)
    {
        _fields._value = value;
    }

    void BsonTreeItem::setType(mongo::BSONType type)
    {
       _fields._type = type;
    }

    mongo::BinDataType BsonTreeItem::binType() const
    {
        return _fields._binType;
    }

    void BsonTreeItem::setBinType(mongo::BinDataType type)
    {
        _fields._binType = type;
    }

    void BsonTreeItem::removeChild(BsonTreeItem *item)
    {
        _items.erase(std::remove_if(_items.begin(), _items.end(), removeIfFound(item)), _items.end());
    }
}
