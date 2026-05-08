#include "mvc/App.hpp"
#include "monitor/repository/sample_repository.hpp"
#include "monitor/repository/order_repository.hpp"
#include "monitor/repository/production_repository.hpp"
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace mvc {

App::App(AppConfig config) : config_(std::move(config)) {}

void App::run() {
    // Initialize repositories to verify DataStore access at startup
    std::string samples_path    = (fs::path(config_.data_dir) / "samples.json").string();
    std::string orders_path     = (fs::path(config_.data_dir) / "orders.json").string();
    std::string productions_path = (fs::path(config_.data_dir) / "productions.json").string();

    // Construct repositories to confirm persistence layer is accessible
    SampleRepository    sample_repo(samples_path);
    OrderRepository     order_repo(orders_path);
    ProductionRepository production_repo(productions_path);

    bool running = true;
    while (running) {
        std::cout << "\n=== SampleOrderSystem ===\n"
                  << " 1. 시료 관리\n"
                  << " 2. 주문 접수\n"
                  << " 3. 주문 처리\n"
                  << " 4. 모니터링\n"
                  << " 5. 출고 처리\n"
                  << " 6. 생산 라인\n"
                  << " 0. 종료\n"
                  << "> ";

        std::string input;
        if (!std::getline(std::cin, input)) {
            break;
        }

        if      (input == "1") { menu_sample_management(); }
        else if (input == "2") { menu_order_reception(); }
        else if (input == "3") { menu_order_processing(); }
        else if (input == "4") { menu_monitoring(); }
        else if (input == "5") { menu_release_processing(); }
        else if (input == "6") { menu_production_line(); }
        else if (input == "0") { running = false; }
        else                   { std::cout << "잘못된 입력입니다.\n"; }
    }
}

void App::menu_sample_management() {
    std::cout << "준비 중\n";
}

void App::menu_order_reception() {
    std::cout << "준비 중\n";
}

void App::menu_order_processing() {
    std::cout << "준비 중\n";
}

void App::menu_monitoring() {
    std::cout << "준비 중\n";
}

void App::menu_release_processing() {
    std::cout << "준비 중\n";
}

void App::menu_production_line() {
    std::cout << "준비 중\n";
}

} // namespace mvc
