#pragma once
#include "data_store.hpp"
#include "monitor/repository/sample_repository.hpp"
#include "monitor/repository/order_repository.hpp"
#include "monitor/repository/production_repository.hpp"
#include <string>
#include <vector>

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

    // 쓰기용 DataStore (persistence layer)
    DataStore sample_store_;
    DataStore order_store_;   // 주문 쓰기용

    // 읽기용 Repository (monitor layer)
    SampleRepository     sample_repo_;
    OrderRepository      order_repo_;
    ProductionRepository production_repo_;

    // 메인 메뉴 핸들러
    void menu_sample_management();
    void menu_order_reception();
    void menu_order_processing();
    void menu_monitoring();
    void menu_release_processing();
    void menu_production_line();

    // 시료 관리 서브 메서드
    void sample_register();
    void sample_list();
    void sample_search();

    // 주문 접수 서브 메서드
    void order_reception();

    // 출력 헬퍼 (static)
    static void print_sample_detail(const Sample& s);
    static void print_sample_table(const std::vector<Sample>& samples);
};

} // namespace mvc
