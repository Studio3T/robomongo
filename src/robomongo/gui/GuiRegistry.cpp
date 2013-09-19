#include "robomongo/gui/GuiRegistry.h"

#include <QApplication>
#include <QStyle>
#include <QIcon>

namespace Robomongo
{

    /**
     * @brief This is a private constructor, because GuiRegistry is a singleton
     */
    GuiRegistry::GuiRegistry()
    {
    }

    /**
     * @brief Functions that provide access to various icons
     */
    void GuiRegistry::setAlternatingColor(QAbstractItemView *view)
    {
    #if defined(Q_OS_MAC)
        view->setAlternatingRowColors(true);

        QPalette p = view->palette();
        p.setColor(QPalette::AlternateBase, QColor(243, 246, 250));
        view->setPalette(p);
    #endif
    }

    const QIcon &GuiRegistry::serverIcon() const
    {
        static const QIcon serverIc = QIcon(":/robomongo/icons/server_16x16.png");
        return serverIc;
    }

    const QIcon &GuiRegistry::openIcon() const
    {
        static const QIcon openIc = qApp->style()->standardIcon(QStyle::SP_DialogOpenButton);
        //static const QIcon openIc = QIcon(":/robomongo/icons/open_32x32.png");
        return openIc;
    }

    const QIcon &GuiRegistry::saveIcon() const
    {
        static const QIcon saveIc = qApp->style()->standardIcon(QStyle::SP_DialogSaveButton);
        // static const QIcon saveIc = QIcon(":/robomongo/icons/save_32x32.png");
        return saveIc;
    }

    const QIcon &GuiRegistry::databaseIcon() const
    {
        static const QIcon databaseIc = QIcon(":/robomongo/icons/database_16x16.png");
        return databaseIc;
    }

    const QIcon &GuiRegistry::collectionIcon() const
    {
        static const QIcon collectionIc = QIcon(":/robomongo/icons/collection_16x16.png");
        return collectionIc;
    }

    const QIcon &GuiRegistry::indexIcon() const
    {
        static const QIcon collectionIc = QIcon(":/robomongo/icons/index_16x16.png");
        return collectionIc;
    }

    const QIcon &GuiRegistry::userIcon() const
    {
        static const QIcon userIc = QIcon(":/robomongo/icons/user_16x16.png");
        return userIc;
    }

    const QIcon &GuiRegistry::functionIcon() const
    {
        static const QIcon functionIc = QIcon(":/robomongo/icons/function_16x16.png");
        return functionIc;
    }

    const QIcon &GuiRegistry::maximizeIcon() const
    {
        static const QIcon maximizeIc = QIcon(":/robomongo/icons/maximize.gif");
        return maximizeIc;
    }

    const QIcon &GuiRegistry::maximizeHighlightedIcon() const
    {
        static const QIcon maximizeHighlightedIc = QIcon(":/robomongo/icons/maximize_highlighted_16x16.png");
        return maximizeHighlightedIc;
    }

    const QIcon &GuiRegistry::textIcon() const
    {
        static const QIcon textIc = QIcon(":/robomongo/icons/text_16x16.png");
        return textIc;
    }

    const QIcon &GuiRegistry::textHighlightedIcon() const
    {
        static const QIcon textHighlightedIc = QIcon(":/robomongo/icons/text_highlighted_16x16.png");
        return textHighlightedIc;
    }

    const QIcon &GuiRegistry::treeIcon() const
    {
        static const QIcon treeIc = QIcon(":/robomongo/icons/tree_16x16.png");
        return treeIc;
    }

    const QIcon &GuiRegistry::treeHighlightedIcon() const
    {
        static const QIcon treeHighlightedIc = QIcon(":/robomongo/icons/tree_highlighted_16x16.png");
        return treeHighlightedIc;
    }

    const QIcon &GuiRegistry::tableIcon() const
    {
        static const QIcon treeIc = QIcon(":/robomongo/icons/table_16x16.png");
        return treeIc;
    }

    const QIcon &GuiRegistry::tableHighlightedIcon() const
    {
        static const QIcon treeHighlightedIc = QIcon(":/robomongo/icons/table_highlighted_16x16.png");
        return treeHighlightedIc;
    }

    const QIcon &GuiRegistry::customIcon() const
    {
        static const QIcon customIc = QIcon(":/robomongo/icons/custom_16x16.png");
        return customIc;
    }

    const QIcon &GuiRegistry::customHighlightedIcon() const
    {
        static const QIcon customHighlightedIc = QIcon(":/robomongo/icons/custom_highlighted_16x16.png");
        return customHighlightedIc;
    }

    const QIcon &GuiRegistry::rotateIcon() const
    {
        static const QIcon rotateIc = QIcon(":/robomongo/icons/rotate_16x16.png");
        return rotateIc;
    }

    const QIcon &GuiRegistry::visualIcon() const
    {
        static const QIcon visualIc = QIcon(":/robomongo/icons/visual_16x16.png");
        return visualIc;
    }

    const QIcon &GuiRegistry::circleIcon() const
    {
        static const QIcon circleIc = QIcon(":/robomongo/icons/bson_unsupported_16x16.png");
        return circleIc;
    }

    const QIcon &GuiRegistry::bsonArrayIcon() const
    {
        static const QIcon bsonArrayIc = QIcon(":/robomongo/icons/bson_array_16x16.png");
        return bsonArrayIc;
    }


    const QIcon &GuiRegistry::bsonObjectIcon() const
    {
        static const QIcon bsonObjectIc = QIcon(":/robomongo/icons/bson_object_16x16.png");
        return bsonObjectIc;
    }

    const QIcon &GuiRegistry::bsonStringIcon() const
    {
        static const QIcon bsonStringIc = QIcon(":/robomongo/icons/bson_string_16x16.png");
        return bsonStringIc;
    }

    const QIcon &GuiRegistry::folderIcon() const
    {
        static const QIcon folderIc = qApp->style()->standardIcon(QStyle::SP_DirClosedIcon);
        return folderIc;
    }

    const QIcon &GuiRegistry::bsonIntegerIcon() const
    {
        static const QIcon bsonIntegerIc = QIcon(":/robomongo/icons/bson_integer_16x16.png");
        return bsonIntegerIc;
    }

    const QIcon &GuiRegistry::bsonDoubleIcon() const
    {
        static const QIcon bsonDoubleIc = QIcon(":/robomongo/icons/bson_double_16x16.png");
        return bsonDoubleIc;
    }

    const QIcon &GuiRegistry::bsonDateTimeIcon() const
    {
        static const QIcon bsonDateTimeIc = QIcon(":/robomongo/icons/bson_datetime_16x16.png");
        return bsonDateTimeIc;
    }

    const QIcon &GuiRegistry::bsonBinaryIcon() const
    {
        static const QIcon bsonBinaryIc = QIcon(":/robomongo/icons/bson_binary_16x16.png");
        return bsonBinaryIc;
    }

    const QIcon &GuiRegistry::bsonNullIcon() const
    {
        static const QIcon bsonNullIc = QIcon(":/robomongo/icons/bson_null_16x16.png");
        return bsonNullIc;
    }

    const QIcon &GuiRegistry::bsonBooleanIcon() const
    {
        static const QIcon bsonBooleanIc = QIcon(":/robomongo/icons/bson_bool_16x16.png");
        return bsonBooleanIc;
    }

    const QIcon &GuiRegistry::noMarkIcon() const
    {
        static const QIcon noMarkIc = QIcon(":/robomongo/icons/no_mark_16x16.png");
        return noMarkIc;
    }

    const QIcon &GuiRegistry::yesMarkIcon() const
    {
        static const QIcon yesMarkIc = QIcon(":/robomongo/icons/yes_mark_16x16.png");
        return yesMarkIc;
    }

    const QIcon &GuiRegistry::timeIcon() const
    {
        static const QIcon timeIc = QIcon(":/robomongo/icons/time_16x16.png");
        return timeIc;
    }

    const QIcon &GuiRegistry::keyIcon() const
    {
        static const QIcon keyIc = QIcon(":/robomongo/icons/key_16x16.png");
        return keyIc;
    }

    const QBrush &GuiRegistry::typeBrush() const
    {
        static const QBrush typeBrush = QBrush(QColor(150,150, 150));
        return typeBrush;
    }

    const QIcon &GuiRegistry::leftIcon() const
    {
        static const QIcon leftIc = QIcon(":/robomongo/icons/left_16x16.png");
        return leftIc;
    }

    const QIcon &GuiRegistry::rightIcon() const
    {
        static const QIcon rightIc = QIcon(":/robomongo/icons/right_16x16.png");

        return rightIc;
    }

    const QIcon &GuiRegistry::mongodbIcon() const
    {
        static const QIcon mongodbIc = QIcon(":/robomongo/icons/mongodb_16x16.png");
        return mongodbIc;
    }

    const QIcon &GuiRegistry::connectIcon() const
    {
        static const QIcon connectIc = QIcon(":/robomongo/icons/connect_24x24.png");
        return connectIc;
    }

    const QIcon &GuiRegistry::executeIcon() const
    {
        static const QIcon executeIc = QIcon(":/robomongo/icons/execute_24x24.png");
        return executeIc;
    }

    const QIcon &GuiRegistry::stopIcon() const
    {
        static const QIcon stopIc = QIcon(":/robomongo/icons/stop_24x24.png");
        return stopIc;
    }

    const QIcon &GuiRegistry::mainWindowIcon() const
    {
        static const QIcon mainWindowIc = QIcon(":/robomongo/icons/main_window_icon.png");
        return mainWindowIc;
    }

    const QFont &GuiRegistry::font() const
    {
        
#if defined(Q_OS_MAC)
        static const QFont textFont = QFont("Monaco",12);
#elif defined(Q_OS_UNIX)
        static QFont textFont = QFont("Monospace");
        textFont.setFixedPitch(true);        
#elif defined(Q_OS_WIN)
        static const QFont textFont = QFont("Courier",10);
#endif
        return textFont;
    }
}
