#pragma once
#include <QDialog>
QT_BEGIN_NAMESPACE
class QTabWidget;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE
namespace Robomongo
{
    class ExplorerCollectionTreeItem;
    class EditIndexDialog: public QDialog
    {
        Q_OBJECT
    public:
        typedef QDialog BaseClass;
        enum
        {
            HeightWidget = 320,
            WidthWidget = 480
        };
        explicit EditIndexDialog(QWidget *parent,ExplorerCollectionTreeItem * const item);
    private:
       QLineEdit *_nameLineEdit;
       QTabWidget *_mainTab;
       QTextEdit *_jsonText;
       ExplorerCollectionTreeItem * const _item;
    };
}
