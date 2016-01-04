#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>

#include <locale.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);


    qDebug() << "Hello!";

    return app.exec();
}

