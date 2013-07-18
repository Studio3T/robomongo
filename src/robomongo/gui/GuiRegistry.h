#pragma once

#include <QIcon>
#include <QBrush>
#include <QAbstractItemView>

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

        void setAlternatingColor(QAbstractItemView *view);

        /**
         * @brief Functions that provide access to various icons
         */
        const QIcon &serverIcon()const;
        const QIcon &saveIcon()const;
        const QIcon &openIcon()const;
        const QIcon &databaseIcon()const;
        const QIcon &collectionIcon()const;
        const QIcon &indexIcon()const;
        const QIcon &userIcon()const;
        const QIcon &functionIcon()const;
        const QIcon &maximizeIcon()const;
        const QIcon &maximizeHighlightedIcon()const;
        const QIcon &textIcon()const;
        const QIcon &textHighlightedIcon()const;
        const QIcon &treeIcon()const;
        const QIcon &treeHighlightedIcon()const;
        const QIcon &customIcon()const;
        const QIcon &customHighlightedIcon()const;
        const QIcon &rotateIcon()const;
        const QIcon &visualIcon()const;
        const QIcon &folderIcon()const;
        const QIcon &circleIcon()const;
        const QIcon &leftIcon()const;
        const QIcon &rightIcon()const;
        const QIcon &mongodbIcon()const;
        const QIcon &connectIcon()const;
        const QIcon &executeIcon()const;
        const QIcon &stopIcon()const;
        const QIcon &mainWindowIcon()const;
        const QIcon &bsonObjectIcon()const;
        const QIcon &bsonArrayIcon()const;
        const QIcon &bsonStringIcon()const;
        const QIcon &bsonIntegerIcon()const;
        const QIcon &bsonDoubleIcon()const;
        const QIcon &bsonDateTimeIcon()const;
        const QIcon &bsonBinaryIcon()const;
        const QIcon &bsonNullIcon()const;
        const QIcon &bsonBooleanIcon()const;
        const QIcon &noMarkIcon()const;
        const QIcon &yesMarkIcon()const;
        const QIcon &timeIcon()const;
        const QIcon &keyIcon()const;

        const QBrush &typeBrush() const;

        const QFont &font() const;
    private:
        /**
         * @brief Private, because this is singleton
         */
        GuiRegistry();
        MainWindow *_mainWindow;
    };
}
