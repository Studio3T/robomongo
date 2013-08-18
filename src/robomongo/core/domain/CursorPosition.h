#pragma once

namespace Robomongo
{
    class CursorPosition
    {
    public:
        CursorPosition();
        CursorPosition(int line, int column);

        bool isNull() const { return _isNull; }
        int line() const { return _line; }
        int column() const { return _column; }

    private:
        const bool _isNull;
        const int _line;
        const int _column;
    };
}
