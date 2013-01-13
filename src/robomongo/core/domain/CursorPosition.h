#pragma once

namespace Robomongo
{
    class CursorPosition
    {
    public:
        CursorPosition() :
            _isNull(true) {}

        CursorPosition(int line, int column) :
            _isNull(false),
            _line(line),
            _column(column) {}

        bool isNull() const { return _isNull; }
        int line() const { return _line; }
        int column() const { return _column; }

    private:
        bool _isNull;
        int _line;
        int _column;
    };
}
