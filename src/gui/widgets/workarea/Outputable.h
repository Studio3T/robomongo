#ifndef OUTPUTABLE_H
#define OUTPUTABLE_H

namespace Robomongo
{
    class Outputable
    {
    public:
        Outputable() :
            _isTextModeSupported(false),
            _isTreeModeSupported(false),
            _isCustomModeSupported(false) { }

        virtual void showText() { }
        virtual void showTree() { }
        virtual void showCustom() { }

        bool isTextModeSupported() const { return _isTextModeSupported; }
        bool isTreeModeSupported() const { return _isTreeModeSupported; }
        bool isCustomModeSupported() const { return _isCustomModeSupported; }

        bool setTextModeSupported(bool supported) { _isTextModeSupported = supported; }
        bool isTreeModeSupported(bool supported) { _isTreeModeSupported = supported; }
        bool isCustomModeSupported(bool supported) { _isCustomModeSupported = supported; }

    private:

        bool _isTextModeSupported;
        bool _isTreeModeSupported;
        bool _isCustomModeSupported;
    };
}

#endif // OUTPUTABLE_H
