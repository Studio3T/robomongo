#include <QApplication>
#include <QMainWindow>

#include <parser.h>
#include <serializer.h>
#include <Qsci/qsciscintilla.h>

// TODO: The following include should NOT work (remove this folder from include dirs)
#include <gui/editors/JSLexer.h>

int main(int argc, char *argv[], char** envp)
{
    QApplication app(argc, argv);

    QJson::Parser parser;
    Robomongo::JSLexer lexer;

    QsciScintilla editor;

    QMainWindow win ;
    win.show();

    return app.exec();
}
