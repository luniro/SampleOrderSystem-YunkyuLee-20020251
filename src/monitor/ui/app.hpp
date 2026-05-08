#pragma once
#include <string>

struct DataPaths {
    std::string samples;
    std::string orders;
    std::string productions;
};

class App {
public:
    explicit App(DataPaths paths);
    void run();

private:
    void show_samples();
    void show_orders();
    void show_orders_by_status();
    void show_productions();

    DataPaths paths_;
};
