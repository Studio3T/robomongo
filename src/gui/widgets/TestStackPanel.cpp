#include "TestStackPanel.h"
#include <QHBoxLayout>
#include <QScrollArea>
#include <QTextStream>
#include <QGraphicsScene>
#include <QtGui>

using namespace Robomongo;

TestStackPanel::TestStackPanel(QWidget *parent) :
    QWidget(parent)
{
    //setFixedSize(500, 600);

    QFile file("/home/dmitry/dev/tmp/JSONeditor.js");
    if(!file.open(QIODevice::ReadOnly))
        return;

    QTextStream in(&file);
    QString esprima = in.readAll();

    QFont textFont = font();
    textFont.setFamily("Monospace");
    textFont.setFixedPitch(true);

    _queryText1 = new RoboScintilla;
    _queryText1->setFixedHeight(300);
    _queryText1->setFrameShape(QFrame::NoFrame);
    _queryText1->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _queryText1->setText(esprima);
    _queryText1->setMarginWidth(1, 0); // to hide left gray column
    _queryText1->setFont(textFont);
    _queryText1->setFixedHeight(600);
    _queryText1->setFocusPolicy( Qt::StrongFocus );
    _queryText1->verticalScrollBar()->setStyleSheet(
        "QScrollBar:vertical { width: 8px; background-color:red; }");

    _queryText2 = new RoboScintilla;
    _queryText2->setFrameShape(QFrame::NoFrame);
    _queryText2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _queryText2->setText("esprima");
    _queryText2->setFont(textFont);
    _queryText2->setFixedHeight(600);
    _queryText2->setFocusPolicy( Qt::StrongFocus );
    //_queryText2->setFixedHeight(300);

    updateSize();

    QTreeWidget *tree = new QTreeWidget();
    //tree->setAllColumnsShowFocus(false);

    QPalette p = tree->palette();
    p.setColor(QPalette::Highlight, Qt::red);
    tree->setPalette(p);

    tree->setStyleSheet("QTreeWidget::item:selected { border-color:green; background-color:white; "
                       "border-style:outset; border-width:2px; color:black; }");


    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, "hello");
    tree->addTopLevelItem(item);
    tree->setItemWidget(item, -1, _queryText2);

    for (int i = 0; i < 100; i++)
    {
        if (i == 30)
        {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, "hello");
            tree->addTopLevelItem(item);
            tree->setItemWidget(item, 0, _queryText1);
        }

        QTreeWidgetItem *item2 = new QTreeWidgetItem();
        item2->setText(0, "hello2");
        tree->addTopLevelItem(item2);
    }

    tree->setFixedSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
    tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);


    QHBoxLayout *l = new QHBoxLayout;
    l->addWidget(tree, 1);
    setLayout(l);


}

void TestStackPanel::updateSize()
{
    int z = _queryText1->textHeight(0);
    int desc = _queryText1->extraDescent(); //SendScintilla(SCI_TEXTHEIGHT, linenr)
    int asc  = _queryText1->extraAscent();

    QFontMetrics m(_queryText1->font());
    int lineHeight = m.lineSpacing() + 1;

    int numberOfLines = _queryText1->lines();

    int height = numberOfLines * lineHeight + 8;

/*    int maxHeight = 18 * lineHeight + 8;
    if (height > maxHeight)
        height = maxHeight;
        */

    _queryText1->setFixedHeight(height);

}
