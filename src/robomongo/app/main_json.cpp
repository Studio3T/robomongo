#include <QApplication>
#include <QMainWindow>

#include <parser.h>
#include <serializer.h>
#include <Qsci/qsciscintilla.h>

#include <robomongo/gui/editors/JSLexer.h>
#include <mongo/util/exit_code.h>

namespace mongo {
    extern bool isShell;
    void logProcessDetailsForLogRotate() {}
    void exitCleanly(ExitCode code) {}
}

int main(int argc, char *argv[], char** envp)
{
    QApplication app(argc, argv);

    QJson::Parser parser;
    Robomongo::JSLexer lexer;
    Robomongo::JSLexer lexer2z;

    QsciScintilla editor;

    QMainWindow win;
    win.setWindowTitle("Some title");
    win.show();

    return app.exec();
}
