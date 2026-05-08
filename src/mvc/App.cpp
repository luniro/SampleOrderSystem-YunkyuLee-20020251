#include "mvc/App.hpp"
#include "mvc/input_util.hpp"
#include "monitor/ui/table_printer.hpp"
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace mvc {

App::App(AppConfig config)
    : config_(std::move(config))
    , sample_store_((fs::path(config_.data_dir) / "samples.json").string())
    , sample_repo_((fs::path(config_.data_dir) / "samples.json").string())
    , order_repo_((fs::path(config_.data_dir) / "orders.json").string())
    , production_repo_((fs::path(config_.data_dir) / "productions.json").string())
{}

void App::run() {
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

// ── 메인 메뉴 1: 시료 관리 ──────────────────────────────────────────────────

void App::menu_sample_management() {
    bool loop = true;
    while (loop) {
        std::cout << "\n[ 1. 시료 관리 ]\n"
                  << " 1. 시료 등록\n"
                  << " 2. 시료 조회\n"
                  << " 3. 시료 검색\n"
                  << " 0. 돌아가기\n";
        int choice = InputUtil::read_int("> ", 0, 3);
        switch (choice) {
            case 1: sample_register(); break;
            case 2: sample_list();     break;
            case 3: sample_search();   break;
            case 0: loop = false;      break;
            default: break;
        }
    }
}

// ── 시료 등록 ────────────────────────────────────────────────────────────────

void App::sample_register() {
    // 중복 검사를 위해 현재 목록 로드
    sample_repo_.refresh();
    auto existing = sample_repo_.find_all();

    // 1. 시료 ID
    std::string sample_id;
    while (true) {
        sample_id = InputUtil::read_nonempty("시료 ID: ");
        if (sample_id.empty()) break; // EOF
        bool dup = false;
        for (const auto& s : existing) {
            if (s.sample_id == sample_id) { dup = true; break; }
        }
        if (dup) {
            std::cout << "이미 존재하는 시료 ID입니다. 다시 입력해 주세요.\n";
            continue;
        }
        break;
    }
    if (sample_id.empty()) return;

    // 2. 시료명
    std::string sample_name;
    while (true) {
        sample_name = InputUtil::read_nonempty("시료명: ");
        if (sample_name.empty()) break; // EOF
        bool dup = false;
        for (const auto& s : existing) {
            if (s.sample_name == sample_name) { dup = true; break; }
        }
        if (dup) {
            std::cout << "이미 존재하는 시료명입니다. 다시 입력해 주세요.\n";
            continue;
        }
        break;
    }
    if (sample_name.empty()) return;

    // 3. 평균 생산시간
    double avg_production_time = 0.0;
    while (true) {
        std::string raw = InputUtil::read_nonempty("평균 생산시간(min/ea): ");
        if (raw.empty()) return; // EOF
        try {
            std::size_t pos = 0;
            avg_production_time = std::stod(raw, &pos);
            if (pos != raw.size()) {
                throw std::invalid_argument("trailing chars");
            }
        } catch (const std::invalid_argument&) {
            std::cout << "유효하지 않은 값입니다. 다시 입력해 주세요.\n";
            continue;
        } catch (const std::out_of_range&) {
            std::cout << "유효하지 않은 값입니다. 다시 입력해 주세요.\n";
            continue;
        }
        if (avg_production_time <= 0.0) {
            std::cout << "평균 생산시간은 0보다 커야 합니다. 다시 입력해 주세요.\n";
            continue;
        }
        bool dup = false;
        for (const auto& s : existing) {
            if (s.avg_production_time == avg_production_time) { dup = true; break; }
        }
        if (dup) {
            std::cout << "이미 존재하는 평균 생산시간입니다. 다시 입력해 주세요.\n";
            continue;
        }
        break;
    }

    // 4. 수율
    double yield_rate = 0.0;
    while (true) {
        std::string raw = InputUtil::read_nonempty("수율(0.70~0.99): ");
        if (raw.empty()) return; // EOF
        try {
            std::size_t pos = 0;
            yield_rate = std::stod(raw, &pos);
            if (pos != raw.size()) {
                throw std::invalid_argument("trailing chars");
            }
        } catch (const std::invalid_argument&) {
            std::cout << "유효하지 않은 값입니다. 다시 입력해 주세요.\n";
            continue;
        } catch (const std::out_of_range&) {
            std::cout << "유효하지 않은 값입니다. 다시 입력해 주세요.\n";
            continue;
        }
        if (yield_rate < 0.70 || yield_rate > 0.99) {
            std::cout << "수율은 0.70 이상 0.99 이하여야 합니다. 다시 입력해 주세요.\n";
            continue;
        }
        bool dup = false;
        for (const auto& s : existing) {
            if (s.yield_rate == yield_rate) { dup = true; break; }
        }
        if (dup) {
            std::cout << "이미 존재하는 수율입니다. 다시 입력해 주세요.\n";
            continue;
        }
        break;
    }

    // 5. 현재 재고
    int current_stock = InputUtil::read_int("현재 재고(ea): ", 0, INT_MAX);

    // 저장
    JsonValue rec = JsonValue::object();
    rec["sample_id"]           = JsonValue(sample_id);
    rec["sample_name"]         = JsonValue(sample_name);
    rec["avg_production_time"] = JsonValue(avg_production_time);
    rec["yield_rate"]          = JsonValue(yield_rate);
    rec["current_stock"]       = JsonValue(static_cast<int64_t>(current_stock));
    sample_store_.create(rec);
    sample_repo_.refresh();

    // 결과 출력 (FR-S-03)
    std::cout << "시료가 등록되었습니다.\n"
              << "  시료 ID: " << sample_id << "\n";
}

// ── 시료 조회 ────────────────────────────────────────────────────────────────

void App::sample_list() {
    sample_repo_.refresh();
    auto samples = sample_repo_.find_all();
    if (samples.empty()) {
        std::cout << "등록된 시료가 없습니다.\n";
        return;
    }
    print_sample_table(samples);
}

// ── 시료 검색 ────────────────────────────────────────────────────────────────

void App::sample_search() {
    bool loop = true;
    while (loop) {
        std::cout << "\n검색 방법을 선택하세요:\n"
                  << " 1. 시료명 검색 (부분 일치)\n"
                  << " 2. 시료 ID 검색 (정확 일치)\n"
                  << " 3. 수율 검색 (정확 일치)\n"
                  << " 4. 평균 생산시간 검색 (정확 일치)\n"
                  << " 0. 돌아가기\n";
        int choice = InputUtil::read_int("> ", 0, 4);

        switch (choice) {
            case 0:
                loop = false;
                break;

            case 1: {
                // 시료명 부분 검색
                std::string keyword = InputUtil::read_nonempty("검색어: ");
                if (keyword.empty()) { loop = false; break; } // EOF
                sample_repo_.refresh();
                auto all = sample_repo_.find_all();
                std::vector<Sample> found;
                for (const auto& s : all) {
                    if (s.sample_name.find(keyword) != std::string::npos)
                        found.push_back(s);
                }
                if (found.empty()) {
                    std::cout << "검색 결과가 없습니다.\n";
                } else if (found.size() == 1) {
                    print_sample_detail(found[0]);
                } else {
                    print_sample_table(found);
                }
                break;
            }

            case 2: {
                // 시료 ID 정확 검색
                std::string sid = InputUtil::read_nonempty("시료 ID: ");
                if (sid.empty()) { loop = false; break; } // EOF
                sample_repo_.refresh();
                auto result = sample_repo_.find_by_sample_id(sid);
                if (!result.has_value()) {
                    std::cout << "검색 결과가 없습니다.\n";
                } else {
                    print_sample_detail(result.value());
                }
                break;
            }

            case 3: {
                // 수율 정확 검색
                double target_yield = 0.0;
                while (true) {
                    std::string raw = InputUtil::read_nonempty("수율: ");
                    if (raw.empty()) { loop = false; break; } // EOF
                    try {
                        std::size_t pos = 0;
                        target_yield = std::stod(raw, &pos);
                        if (pos != raw.size()) throw std::invalid_argument("trailing");
                        break;
                    } catch (...) {
                        std::cout << "유효하지 않은 값입니다. 다시 입력해 주세요.\n";
                    }
                }
                if (loop == false) break;
                sample_repo_.refresh();
                auto all = sample_repo_.find_all();
                const Sample* found_ptr = nullptr;
                for (const auto& s : all) {
                    if (s.yield_rate == target_yield) { found_ptr = &s; break; }
                }
                if (!found_ptr) {
                    std::cout << "검색 결과가 없습니다.\n";
                } else {
                    print_sample_detail(*found_ptr);
                }
                break;
            }

            case 4: {
                // 평균 생산시간 정확 검색
                double target_pt = 0.0;
                while (true) {
                    std::string raw = InputUtil::read_nonempty("평균 생산시간(min/ea): ");
                    if (raw.empty()) { loop = false; break; } // EOF
                    try {
                        std::size_t pos = 0;
                        target_pt = std::stod(raw, &pos);
                        if (pos != raw.size()) throw std::invalid_argument("trailing");
                        break;
                    } catch (...) {
                        std::cout << "유효하지 않은 값입니다. 다시 입력해 주세요.\n";
                    }
                }
                if (loop == false) break;
                sample_repo_.refresh();
                auto all = sample_repo_.find_all();
                const Sample* found_ptr = nullptr;
                for (const auto& s : all) {
                    if (s.avg_production_time == target_pt) { found_ptr = &s; break; }
                }
                if (!found_ptr) {
                    std::cout << "검색 결과가 없습니다.\n";
                } else {
                    print_sample_detail(*found_ptr);
                }
                break;
            }

            default:
                break;
        }
    }
}

// ── 출력 헬퍼 ────────────────────────────────────────────────────────────────

void App::print_sample_detail(const Sample& s) {
    std::cout << "[ 시료 상세 ]\n"
              << "  시료 ID       : " << s.sample_id << "\n"
              << "  시료명        : " << s.sample_name << "\n"
              << "  평균 생산시간 : "
              << std::fixed << std::setprecision(1) << s.avg_production_time << " min/ea\n"
              << "  수율          : " << static_cast<int>(s.yield_rate * 100) << " %\n"
              << "  현재 재고     : " << s.current_stock << " ea\n";
}

void App::print_sample_table(const std::vector<Sample>& samples) {
    TablePrinter tp({"시료 ID", "시료명", "평균생산시간(min/ea)", "수율(%)", "현재재고(ea)"});
    for (const auto& s : samples) {
        std::ostringstream apt;
        apt << std::fixed << std::setprecision(1) << s.avg_production_time;
        std::string yield_str = std::to_string(static_cast<int>(s.yield_rate * 100));
        std::string stock_str = std::to_string(s.current_stock);
        tp.add_row({s.sample_id, s.sample_name, apt.str(), yield_str, stock_str});
    }
    tp.print_paged();
}

// ── 메인 메뉴 2~6 (stub) ─────────────────────────────────────────────────────

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
