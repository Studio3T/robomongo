#ifndef GUIREGISTRY_H
#define GUIREGISTRY_H

#include <QIcon>

namespace Robomongo
{
    /**
     * @brief GuiRegistry is a simple registry-like singleton, that provides access to
     *        to another various singletons (including access to the data that is stored in
     *        resources (i.e. gui.qrc) and caches it, if needed)
     */
    class GuiRegistry
    {
    public:

        /**
         * @brief Returns single instance of GuiRegistry
         */
        static GuiRegistry &instance()
        {
            static GuiRegistry _instance;
            return _instance;
        }

        /**
         * @brief Functions that provide access to various icons
         */
        QIcon serverIcon();
        QIcon databaseIcon();
        QIcon collectionIcon();
        QIcon maximizeIcon();
        QIcon textIcon();
        QIcon treeIcon();
        QIcon rotateIcon();
        QIcon visualIcon();
        QIcon folderIcon();
        QIcon circleIcon();
        QIcon leftIcon();
        QIcon rightIcon();
        QIcon mongodbIcon();
        QIcon bsonObjectIcon();
        QIcon bsonArrayIcon();
        QIcon bsonStringIcon();
        QIcon bsonIntegerIcon();
        QIcon bsonDoubleIcon();
        QIcon bsonDateTimeIcon();
        QIcon bsonBinaryIcon();
        QIcon bsonNullIcon();
        QIcon bsonBooleanIcon();

        QBrush typeBrush();


    private:
        /**
         * @brief Private constructor, because this is singleton
         */
        GuiRegistry();

        /**
         * @brief Icons
         */
        QIcon _serverIcon;
        QIcon _databaseIcon;
        QIcon _folderIcon;
        QIcon _collectionIcon;
        QIcon _maximizeIcon;
        QIcon _treeIcon;
        QIcon _textIcon;
        QIcon _rotateIcon;
        QIcon _visualIcon;
        QIcon _circleIcon;
        QIcon _leftIcon;
        QIcon _rightIcon;
        QIcon _mongodbIcon;
        QIcon _bsonObjectIcon;
        QIcon _bsonIdIcon;
        QIcon _bsonArrayIcon;
        QIcon _bsonStringIcon;
        QIcon _bsonIntegerIcon;
        QIcon _bsonDoubleIcon;
        QIcon _bsonDateTimeIcon;
        QIcon _bsonBinaryIcon;
        QIcon _bsonNullIcon;
        QIcon _bsonBooleanIcon;

        /**
         * @brief Brushes
         */
        QBrush _typeBrush;
    };
}

#endif // GUIREGISTRY_H
