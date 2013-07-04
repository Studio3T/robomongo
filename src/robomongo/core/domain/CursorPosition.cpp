#include "CursorPosition.h"
namespace Robomongo
{
    CursorPosition::CursorPosition() :
        _isNull(true) {}

    CursorPosition::CursorPosition(int line, int column) :
        _isNull(false),
        _line(line),
        _column(column) {}
}
