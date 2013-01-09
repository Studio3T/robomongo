#pragma once

#include <QIcon>
#include <QBrush>

namespace Robomongo
{
    class MainWindow;

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

        MainWindow *mainWindow() const { return _mainWindow;}
        void setMainWindow(MainWindow *mainWindow) { _mainWindow = mainWindow; }

        /**
         * @brief Functions that provide access to various icons
         */
        QIcon serverIcon();
        QIcon databaseIcon();
        QIcon collectionIcon();
        QIcon maximizeIcon();
        QIcon maximizeHighlightedIcon();
        QIcon textIcon();
        QIcon textHighlightedIcon();
        QIcon treeIcon();
        QIcon treeHighlightedIcon();
        QIcon rotateIcon();
        QIcon visualIcon();
        QIcon folderIcon();
        QIcon circleIcon();
        QIcon leftIcon();
        QIcon rightIcon();
        QIcon mongodbIcon();
        QIcon connectIcon();
        QIcon executeIcon();
        QIcon mainWindowIcon();
        QIcon bsonObjectIcon();
        QIcon bsonArrayIcon();
        QIcon bsonStringIcon();
        QIcon bsonIntegerIcon();
        QIcon bsonDoubleIcon();
        QIcon bsonDateTimeIcon();
        QIcon bsonBinaryIcon();
        QIcon bsonNullIcon();
        QIcon bsonBooleanIcon();
        QIcon noMarkIcon();
        QIcon yesMarkIcon();
        QIcon timeIcon();

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
        QIcon _maximizeHighlightedIcon;
        QIcon _treeIcon;
        QIcon _treeHighlightedIcon;
        QIcon _textIcon;
        QIcon _textHighlightedIcon;
        QIcon _rotateIcon;
        QIcon _visualIcon;
        QIcon _circleIcon;
        QIcon _leftIcon;
        QIcon _rightIcon;
        QIcon _mongodbIcon;
        QIcon _connectIcon;
        QIcon _executeIcon;
        QIcon _mainWindowIcon;
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
        QIcon _noMarkIcon;
        QIcon _yesMarkIcon;
        QIcon _timeIcon;

        /**
         * @brief Brushes
         */
        QBrush _typeBrush;

        MainWindow *_mainWindow;
    };
}
