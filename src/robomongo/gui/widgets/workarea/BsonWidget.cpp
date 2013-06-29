#include "robomongo/gui/widgets/workarea/BsonWidget.h"

#include <QPlainTextEdit>
#include <QStackedWidget>
#include <QHBoxLayout>

#include "robomongo/gui/widgets/workarea/BsonTreeWidget.h"

namespace Robomongo
{

    BsonWidget::BsonWidget(MongoShell *shell, QWidget *parent) : QWidget(parent),
        _shell(shell)
    {
        _bsonTree = new BsonTreeWidget(_shell);

        QHBoxLayout *hlayout = new QHBoxLayout;
        hlayout->setSpacing(0);
        hlayout->setMargin(0);
        hlayout->addWidget(_bsonTree);
        setLayout(hlayout);
    }

    void BsonWidget::setDocuments(const QList<MongoDocumentPtr> &documents, const MongoQueryInfo &queryInfo)
    {
        _bsonTree->setDocuments(documents, queryInfo);
    }
}
