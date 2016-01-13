#include <QApplication>
#include <QMainWindow>

#include <parser.h>
#include <serializer.h>
#include <Qsci/qsciscintilla.h>

// TODO: The following include should NOT work (remove this folder from include dirs)
#include <robomongo/gui/editors/JSLexer.h>

int main(int argc, char *argv[], char** envp)
{
    QApplication app(argc, argv);

    QJson::Parser parser;
    Robomongo::JSLexer lexer;
    Robomongo::JSLexer lexer2z;

    QsciScintilla editor;

    QMainWindow win ;
    win.show();

    return app.exec();
}
