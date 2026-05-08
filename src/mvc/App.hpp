#pragma once
#include <string>

namespace mvc {

struct AppConfig {
    std::string data_dir = ".";
};

class App {
public:
    explicit App(AppConfig config);
    void run();

private:
    AppConfig config_;

    void menu_sample_management();
    void menu_order_reception();
    void menu_order_processing();
    void menu_monitoring();
    void menu_release_processing();
    void menu_production_line();
};

} // namespace mvc
