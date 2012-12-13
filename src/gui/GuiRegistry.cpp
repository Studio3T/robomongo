#include <QApplication>
#include <QStyle>
#include <QIcon>

#include "GuiRegistry.h"

using namespace Robomongo;

/**
 * @brief This is a private constructor, because GuiRegistry is a singleton
 */
GuiRegistry::GuiRegistry()
{
}

/**
 * @brief Functions that provide access to various icons
 */
QIcon GuiRegistry::serverIcon()
{
    if (_serverIcon.isNull())
        _serverIcon = QIcon(":/robomongo/icons/server_16x16.gif");

    return _serverIcon;
}

QIcon GuiRegistry::databaseIcon()
{
    if (_databaseIcon.isNull())
        _databaseIcon = QIcon(":/robomongo/icons/database_16x16.png");

    return _databaseIcon;
}

QIcon GuiRegistry::collectionIcon()
{
    if (_collectionIcon.isNull())
        _collectionIcon = QIcon(":/robomongo/icons/collection_16x16.png");

    return _collectionIcon;
}

QIcon GuiRegistry::maximizeIcon()
{
    if (_maximizeIcon.isNull())
        _maximizeIcon = QIcon(":/robomongo/icons/maximize.gif");

    return _maximizeIcon;
}

QIcon GuiRegistry::textIcon()
{
    if (_textIcon.isNull())
        _textIcon = QIcon(":/robomongo/icons/text_16x16.png");

    return _textIcon;
}

QIcon GuiRegistry::treeIcon()
{
    if (_treeIcon.isNull())
        _treeIcon = QIcon(":/robomongo/icons/tree_16x16.png");

    return _treeIcon;
}

QIcon GuiRegistry::visualIcon()
{
    if (_visualIcon.isNull())
        _visualIcon = QIcon(":/robomongo/icons/visual_16x16.png");

    return _visualIcon;
}

QIcon GuiRegistry::circleIcon()
{
    if (_circleIcon.isNull())
        _circleIcon = QIcon(":/robomongo/icons/bson_unsupported_16x16.png");

    return _circleIcon;
}

QIcon GuiRegistry::bsonArrayIcon()
{
    if (_bsonArrayIcon.isNull())
        _bsonArrayIcon = QIcon(":/robomongo/icons/bson_array_16x16.png");

    return _bsonArrayIcon;
}


QIcon GuiRegistry::bsonObjectIcon()
{
    if (_bsonObjectIcon.isNull())
        _bsonObjectIcon = QIcon(":/robomongo/icons/bson_object_16x16.png");

    return _bsonObjectIcon;
}

QIcon GuiRegistry::bsonStringIcon()
{
    if (_bsonStringIcon.isNull())
        _bsonStringIcon = QIcon(":/robomongo/icons/bson_string_16x16.png");

    return _bsonStringIcon;
}

QIcon GuiRegistry::folderIcon()
{
    if (_folderIcon.isNull())
        _folderIcon = qApp->style()->standardIcon(QStyle::SP_DirClosedIcon);

    return _folderIcon;
}

QIcon GuiRegistry::bsonIntegerIcon()
{
    if (_bsonIntegerIcon.isNull())
        _bsonIntegerIcon = QIcon(":/robomongo/icons/bson_integer_16x16.png");

    return _bsonIntegerIcon;
}

QIcon GuiRegistry::bsonDoubleIcon()
{
    if (_bsonDoubleIcon.isNull())
        _bsonDoubleIcon = QIcon(":/robomongo/icons/bson_double_16x16.png");

    return _bsonDoubleIcon;
}

QIcon GuiRegistry::bsonDateTimeIcon()
{
    if (_bsonDateTimeIcon.isNull())
        _bsonDateTimeIcon = QIcon(":/robomongo/icons/bson_datetime_16x16.png");

    return _bsonDateTimeIcon;
}

QIcon GuiRegistry::bsonBinaryIcon()
{
    if (_bsonBinaryIcon.isNull())
        _bsonBinaryIcon = QIcon(":/robomongo/icons/bson_binary_16x16.png");

    return _bsonBinaryIcon;
}

QIcon GuiRegistry::bsonNullIcon()
{
    if (_bsonNullIcon.isNull())
        _bsonNullIcon = QIcon(":/robomongo/icons/bson_null_16x16.png");

    return _bsonNullIcon;
}

QIcon GuiRegistry::bsonBooleanIcon()
{
    if (_bsonBooleanIcon.isNull())
        _bsonBooleanIcon = QIcon(":/robomongo/icons/bson_bool_16x16.png");

    return _bsonBooleanIcon;
}

QBrush GuiRegistry::typeBrush()
{
    if (_typeBrush.style() == Qt::NoBrush)
        _typeBrush = QBrush(QColor(150,150, 150));

    return _typeBrush;
}

QIcon GuiRegistry::leftIcon()
{
    if (_leftIcon.isNull())
        _leftIcon = QIcon(":/robomongo/icons/left_16x16.png");

    return _leftIcon;
}

QIcon GuiRegistry::rightIcon()
{
    if (_rightIcon.isNull())
        _rightIcon = QIcon(":/robomongo/icons/right_16x16.png");

    return _rightIcon;
}

