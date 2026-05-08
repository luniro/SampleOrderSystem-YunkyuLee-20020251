#include "mvc/App.hpp"

int main() {
    mvc::AppConfig config;
    config.data_dir = ".";
    mvc::App app(config);
    app.run();
    return 0;
}
