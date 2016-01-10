#include <QApplication>
#include <QMainWindow>

#include <parser.h>
#include <serializer.h>

int main(int argc, char *argv[], char** envp)
{
    QApplication app(argc, argv);

    QJson::Parser parser;

    QMainWindow win ;
    win.show();

    return app.exec();
}
