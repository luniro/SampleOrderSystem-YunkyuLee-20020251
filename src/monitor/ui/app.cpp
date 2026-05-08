#include "ui/app.hpp"
#include "ui/table_printer.hpp"
#include "repository/sample_repository.hpp"
#include "repository/order_repository.hpp"
#include "repository/production_repository.hpp"
#include <cstdio>
#include <iostream>
#include <string>

static std::string fmt_double(double v, int decimals = 2) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, v);
    return buf;
}

static const char* order_status_name(int status) {
    switch (status) {
        case 0: return "Reserved";
        case 1: return "Rejected";
        case 2: return "Producing";
        case 3: return "Confirmed";
        case 4: return "Release";
        default: return "Unknown";
    }
}

App::App(DataPaths paths) : paths_(std::move(paths)) {}

void App::run() {
    while (true) {
        std::cout << "\n=== DataMonitor ===\n"
                  << "1. 시료(Sample) 목록\n"
                  << "2. 주문(Order) 목록\n"
                  << "3. 주문(Order) 상태별 조회\n"
                  << "4. 생산(Production) 현황\n"
                  << "0. 종료\n"
                  << "> ";

        int choice;
        if (!(std::cin >> choice)) break;

        switch (choice) {
            case 1: show_samples();          break;
            case 2: show_orders();           break;
            case 3: show_orders_by_status(); break;
            case 4: show_productions();      break;
            case 0: return;
            default: std::cout << "잘못된 입력입니다.\n";
        }
    }
}

void App::show_samples() {
    SampleRepository repo(paths_.samples);
    TablePrinter tp({"ID", "sample_id", "sample_name", "avg_time(min)", "yield_rate", "stock"});

    for (const auto& s : repo.find_all()) {
        tp.add_row({
            std::to_string(s.id),
            s.sample_id,
            s.sample_name,
            fmt_double(s.avg_production_time),
            fmt_double(s.yield_rate),
            std::to_string(s.current_stock)
        });
    }
    std::cout << '\n';
    tp.print();
}

void App::show_orders() {
    OrderRepository repo(paths_.orders);
    TablePrinter tp({"ID", "order_number", "sample_id", "customer", "qty", "status"});

    for (const auto& o : repo.find_all()) {
        tp.add_row({
            std::to_string(o.id),
            o.order_number,
            o.sample_id,
            o.customer_name,
            std::to_string(o.order_quantity),
            order_status_name(o.order_status)
        });
    }
    std::cout << '\n';
    tp.print();
}

void App::show_orders_by_status() {
    std::cout << "상태 코드 (0=Reserved 1=Rejected 2=Producing 3=Confirmed 4=Release): ";
    int status;
    if (!(std::cin >> status)) return;

    OrderRepository repo(paths_.orders);
    TablePrinter tp({"ID", "order_number", "sample_id", "customer", "qty"});

    for (const auto& o : repo.find_by_status(status)) {
        tp.add_row({
            std::to_string(o.id),
            o.order_number,
            o.sample_id,
            o.customer_name,
            std::to_string(o.order_quantity)
        });
    }
    std::cout << '\n';
    tp.print();
}

void App::show_productions() {
    ProductionRepository repo(paths_.productions);
    TablePrinter tp({"ID", "order_number", "sample_name", "qty", "shortage", "produced", "ordered_at", "est_complete"});

    for (const auto& p : repo.find_all()) {
        tp.add_row({
            std::to_string(p.id),
            p.order_number,
            p.sample_name,
            std::to_string(p.order_quantity),
            std::to_string(p.shortage),
            std::to_string(p.actual_production),
            p.ordered_at,
            p.estimated_completion
        });
    }
    std::cout << '\n';
    tp.print();
}
