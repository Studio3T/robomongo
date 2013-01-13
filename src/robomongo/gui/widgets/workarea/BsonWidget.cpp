#include "robomongo/gui/widgets/workarea/BsonWidget.h"

#include <QPlainTextEdit>
#include <QStackedWidget>

#include "robomongo/gui/widgets/workarea/BsonTreeWidget.h"

using namespace Robomongo;

BsonWidget::BsonWidget(QWidget *parent) : QWidget(parent)
{
    _bsonTree = new BsonTreeWidget;

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setSpacing(0);
	hlayout->setMargin(0);
    hlayout->addWidget(_bsonTree);
	setLayout(hlayout);
}

void BsonWidget::setDocuments(const QList<MongoDocumentPtr> &documents)
{
	_bsonTree->setDocuments(documents);
}
