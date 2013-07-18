#pragma once
#include <QDialog>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
QT_END_NAMESPACE
namespace Robomongo
{
    class FindFrame;
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
        QString indexName() const;
        QString getInputText()const;
        bool isUnique() const;
        bool isBackGround() const;
        bool isDropDuplicates() const;
        bool isSparce()const;
        QString expireAfter() const;
        QString defaultLanguage() const;
        QString languageOverride() const;
        QString textWeights() const;
    public Q_SLOTS:
        virtual void accept();
    private:
       QWidget* createBasicTab(); 
       QWidget* createAdvancedTab();
       QWidget* createTextSearchTab(); 
       QLineEdit *_nameLineEdit;
       FindFrame *_jsonText;
       ExplorerCollectionTreeItem * const _item;
       QCheckBox *_uniqueCheckBox;

       QCheckBox *_dropDuplicates;
       QCheckBox *_backGroundCheckBox;
       QCheckBox *_sparceCheckBox;
       QLineEdit *_expireAfterLineEdit;

       QLineEdit *_defaultLanguageLineEdit;
       QLineEdit *_languageOverrideLineEdit;
       QLineEdit *_textWeightsLineEdit;
    };
}
