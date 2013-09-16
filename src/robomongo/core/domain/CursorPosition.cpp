#include "robomongo/core/domain/CursorPosition.h"

namespace Robomongo
{
    CursorPosition::CursorPosition() :
        _isNull(true),
        _line(-1),
        _column(-1) {}

    CursorPosition::CursorPosition(int line, int column) :
        _isNull(false),
        _line(line),
        _column(column) {}
}
