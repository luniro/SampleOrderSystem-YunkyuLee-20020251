#include "ui/console.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace Console {

int get_height() {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h == INVALID_HANDLE_VALUE || h == nullptr) {
        return 24;
    }
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (!GetConsoleScreenBufferInfo(h, &csbi)) {
        return 24;
    }
    int height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    return (height > 0) ? height : 24;
#else
    return 24;
#endif
}

} // namespace Console
