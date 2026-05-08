#include "mvc/App.hpp"
#ifdef _WIN32
#include <windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    mvc::AppConfig config;
    config.data_dir = ".";
    mvc::App app(config);
    app.run();
    return 0;
}
