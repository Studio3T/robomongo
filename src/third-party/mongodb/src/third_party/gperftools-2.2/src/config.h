#if defined(__linux__)
#include "config-10gen-linux.h"
#elif defined(__APPLE__)
#include "config-10gen-macos.h"
#elif defined(_WIN32)
#include "config-10gen-windows.h"
#else
#error "In tree version of TCMalloc not supported on this platform!"
#endif
