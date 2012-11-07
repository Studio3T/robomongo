#include "mainwindow.h"
#include <QLabel>
#include <QHBoxLayout>
#include "qstyle.h"
#include <QLayout>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    setObjectName("Main Window");
    setWindowTitle("Qt");
    //ui->setupUi(this);

    resize(400, 300);

    _tabWidget = new QTabWidget(this);
    _tabWidget->setMovable(true);
    //_tabWidget->resize(400, 300);
    _tabWidget->setTabsClosable(true);
    _tabWidget->setTabToolTip(0, "const QString & tip");



    QHBoxLayout * hlayout = new QHBoxLayout;
    hlayout->setObjectName("Main layout");
    hlayout->setMargin(0);
    hlayout->addWidget(_tabWidget);
    setLayout(hlayout);

    _tabWidget->setFocusPolicy(Qt::NoFocus);
    _tabWidget->setDocumentMode(true);

    _tabWidget->setStyleSheet(

                "QTabWidget::pane { /* The tab widget frame */"
                    "border-top: 2px solid #C2C7CB;"
                "}"

                "QTabWidget::tab-bar {"
                    "left: 5px; /* move to the right by 5px */"
                    "top:1px;"
                "}                "

                "QTabBar::tab {"
                    "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                                "stop: 0 #E1E1E1, stop: 0.4 #DDDDDD,"
                                                "stop: 0.5 #D8D8D8, stop: 1.0 #D3D3D3);"
                    "border: 1px solid #C4C4C3;"
                    "border-bottom-color: #C2C7CB; /* same as the pane color */"
                    "border-top-left-radius: 8px;"
                    "border-top-right-radius: 8px;"
                    "min-width: 8ex;"
                    "padding: 2px;"
                "}                "

                "QTabBar::tab:selected, QTabBar::tab:hover {"
                    "background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
                                                "stop: 0 #fafafa, stop: 0.4 #f4f4f4,"
                                                "stop: 0.5 #e7e7e7, stop: 1.0 #fafafa);"
                "}"

                "QTabBar::tab:selected {"
                    "border-color: #9B9B9B;"
                    "border-bottom-color: #C2C7CB; /* same as pane color */"
                "}"

                "QTabBar::tab:!selected {"
                    "margin-top: 2px; /* make non-selected tabs look smaller */"
                "}                "


                );

    _tabWidget->addTab(new QPushButton("Push me, and then"), "File");
    _tabWidget->addTab(new QLabel("Muahaha2"), "About");

    _tabWidget->setCurrentIndex(_tabWidget->count() - 1);

    setCentralWidget(_tabWidget);

}

MainWindow::~MainWindow()
{
}
