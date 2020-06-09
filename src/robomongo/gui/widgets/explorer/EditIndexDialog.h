#pragma once
#include <QDialog>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QCheckBox;
class QTextEdit;
QT_END_NAMESPACE
#include "robomongo/core/events/MongoEventsInfo.h"

namespace Robomongo
{
    class FindFrame;
    class EditIndexDialog: public QDialog
    {
        Q_OBJECT
    public:
        typedef QDialog BaseClass;
        enum {
            HeightWidget = 320,
            WidthWidget = 480
        };
        explicit EditIndexDialog(
            const IndexInfo &info, const QString &databaseName, 
            const QString &serverAdress, bool const isAddIndex,
            QWidget *parent = nullptr
        );
        IndexInfo info() const;

    public Q_SLOTS:
        virtual void accept();
        void expireStateChanged(int value);

    private:
       QWidget *createBasicTab();
       QWidget *createAdvancedTab();
       QWidget *createTextSearchTab();

       bool const _isAddIndex;
       IndexInfo const _info;
       QLineEdit *_nameLineEdit;
       FindFrame *_jsonText;
       QCheckBox *_uniqueCheckBox;

       QCheckBox *_backGroundCheckBox;
       QCheckBox *_sparceCheckBox;
       QLineEdit *_expireAfterLineEdit;

       QLineEdit *_defaultLanguageLineEdit;
       QLineEdit *_languageOverrideLineEdit;
       FindFrame *_textWeightsLineEdit;
    };
}
