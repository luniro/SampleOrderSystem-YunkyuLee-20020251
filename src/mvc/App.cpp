#include "mvc/App.hpp"
#include "mvc/input_util.hpp"
#include "monitor/ui/table_printer.hpp"
#include "monitor/util/timestamp.hpp"
#include "monitor/util/production_calc.hpp"
#include <algorithm>
#include <climits>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace mvc {

App::App(AppConfig config)
    : config_(std::move(config))
    , sample_store_((fs::path(config_.data_dir) / "samples.json").string())
    , order_store_((fs::path(config_.data_dir) / "orders.json").string())
    , production_store_((fs::path(config_.data_dir) / "productions.json").string())
    , sample_repo_((fs::path(config_.data_dir) / "samples.json").string())
    , order_repo_((fs::path(config_.data_dir) / "orders.json").string())
    , production_repo_((fs::path(config_.data_dir) / "productions.json").string())
{}

void App::run() {
    bool running = true;
    while (running) {
        evaluate_producing_orders();  // FR-L-09/FR-L-10

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
        sample_id = InputUtil::read_nonempty("시료 ID (빈 줄: 취소): ");
        if (sample_id.empty()) break;
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
                std::string keyword = InputUtil::read_nonempty("검색어 (빈 줄: 취소): ");
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
                std::string sid = InputUtil::read_nonempty("시료 ID (빈 줄: 취소): ");
                if (sid.empty()) { loop = false; break; }
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
                    std::string raw = InputUtil::read_nonempty("수율 (빈 줄: 취소): ");
                    if (raw.empty()) { loop = false; break; }
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
                    std::string raw = InputUtil::read_nonempty("평균 생산시간(min/ea) (빈 줄: 취소): ");
                    if (raw.empty()) { loop = false; break; }
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

// ── 메인 메뉴 2: 주문 접수 ──────────────────────────────────────────────────────

void App::menu_order_reception() {
    order_reception();
}

// ── 주문 접수 로직 ────────────────────────────────────────────────────────────

void App::order_reception() {
    // 1. 시료 ID 입력 및 검증 (FR-O-02)
    std::string sample_id;
    std::string sample_name;
    while (true) {
        sample_id = InputUtil::read_nonempty("시료 ID (빈 줄: 취소): ");
        if (sample_id.empty()) return;
        sample_repo_.refresh();
        auto found = sample_repo_.find_by_sample_id(sample_id);
        if (!found.has_value()) {
            std::cout << "존재하지 않는 시료 ID입니다. 다시 입력해 주세요.\n";
            continue;
        }
        sample_name = found.value().sample_name;
        break;
    }

    // 2. 고객명 입력
    std::string customer_name = InputUtil::read_nonempty("고객명: ");
    if (customer_name.empty()) return; // EOF

    // 3. 주문수량 입력 (1 이상)
    int quantity = InputUtil::read_int("주문수량: ", 1, INT_MAX);

    // 4. 확인 화면 출력 (FR-O-03)
    std::cout << "\n[ 주문 확인 ]\n"
              << "  시료: " << sample_name << " (" << sample_id << ")\n"
              << "  고객: " << customer_name << "\n"
              << "  수량: " << quantity << " ea\n\n"
              << " 1. 접수\n"
              << " 0. 취소\n";
    int confirm = InputUtil::read_int("> ", 0, 1);

    // 5. 취소(0): return (FR-O-04)
    if (confirm == 0) {
        return;
    }

    // 6. 접수(1): 주문번호 발행 + 저장 + 결과 출력 (FR-O-05, FR-O-06)

    // 오늘 날짜 문자열 "YYYYMMDD" 산출
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char datebuf[9];
    std::strftime(datebuf, sizeof(datebuf), "%Y%m%d", &tm);
    std::string date_str = datebuf;

    // 오늘 날짜 기존 주문 수 카운팅
    std::string prefix = "ORD-" + date_str + "-";
    int count = 0;
    order_repo_.refresh();
    for (const auto& o : order_repo_.find_all()) {
        if (o.order_number.rfind(prefix, 0) == 0) ++count;
    }

    // 주문번호 생성
    int seq = count + 1;
    std::ostringstream oss;
    oss << prefix;
    if (seq < 1000) {
        oss << std::setfill('0') << std::setw(3) << seq;
    } else {
        oss << seq;
    }
    std::string order_number = oss.str();

    // 저장
    JsonValue rec = JsonValue::object();
    rec["order_number"]   = JsonValue(order_number);
    rec["sample_id"]      = JsonValue(sample_id);
    rec["customer_name"]  = JsonValue(customer_name);
    rec["order_quantity"] = JsonValue(static_cast<int64_t>(quantity));
    rec["order_status"]   = JsonValue(int64_t(Order::STATUS_RESERVED));
    rec["approved_at"]    = JsonValue(nullptr);
    rec["released_at"]    = JsonValue(nullptr);
    order_store_.create(rec);
    order_repo_.refresh();

    // 결과 출력 (FR-O-06)
    std::cout << "주문이 접수되었습니다.\n"
              << "  주문번호: " << order_number << "\n"
              << "  상태    : Reserved\n";
}

// ── 메인 메뉴 3: 주문 처리 ──────────────────────────────────────────────────

void App::menu_order_processing() {
    // 처리 후 목록 재표시: order_process_list가 false를 반환할 때까지 반복
    while (order_process_list()) {
        // 한 건 처리 완료 후 목록 재표시
    }
}

// ── 주문 처리: Reserved 목록 표시 + 선택 ─────────────────────────────────────
// 반환값: true=처리 완료(루프 계속), false=종료(빈 목록 또는 사용자 0 선택)

bool App::order_process_list() {
    order_repo_.refresh();
    auto reserved = order_repo_.find_by_status(Order::STATUS_RESERVED);

    if (reserved.empty()) {
        std::cout << "처리 대기 중인 주문이 없습니다.\n";
        return false;
    }

    // 목록 출력 (FR-A-01)
    TablePrinter tp({"No", "주문번호", "고객", "시료ID", "수량(ea)"});
    for (std::size_t i = 0; i < reserved.size(); ++i) {
        const auto& o = reserved[i];
        tp.add_row({
            std::to_string(i + 1),
            o.order_number,
            o.customer_name,
            o.sample_id,
            std::to_string(o.order_quantity)
        });
    }
    tp.print();

    // 번호 입력 (FR-A-02)
    int choice = InputUtil::read_int(
        "번호를 입력하세요 (0: 돌아가기): ",
        0, static_cast<int>(reserved.size()));

    if (choice == 0) return false;

    order_process_detail(reserved[static_cast<std::size_t>(choice - 1)]);
    return true;
}

// ── 주문 처리: 재고 확인 + 승인/거절/취소 ────────────────────────────────────

void App::order_process_detail(const Order& order) {
    // 1. 가용 재고 산출 (PRD 4.3)
    sample_repo_.refresh();
    auto sample_opt = sample_repo_.find_by_sample_id(order.sample_id);
    if (!sample_opt.has_value()) {
        std::cout << "시료 정보를 찾을 수 없습니다.\n";
        return;
    }
    const Sample& sample = sample_opt.value();
    int64_t current_stock = sample.current_stock;

    order_repo_.refresh();
    int64_t confirmed_sum = 0;
    bool has_producing = false;
    for (const auto& o : order_repo_.find_all()) {
        if (o.sample_id == order.sample_id && o.order_status == Order::STATUS_CONFIRMED)
            confirmed_sum += o.order_quantity;
        if (o.sample_id == order.sample_id && o.order_status == Order::STATUS_PRODUCING)
            has_producing = true;
    }
    int64_t available = std::max(int64_t(0), current_stock - confirmed_sum);

    // 2. 부족분 계산 (PRD 4.3 Case 1/2)
    bool needs_production = (available < order.order_quantity);
    int64_t shortage = 0;
    if (needs_production) {
        if (!has_producing) {
            shortage = order.order_quantity - available;  // Case 1
        } else {
            shortage = order.order_quantity;              // Case 2
        }
    }

    // 3. 실 생산량·소요 시간 산출
    int64_t actual_prod = ProductionCalc::actual_production(shortage, sample.yield_rate);
    double  est_minutes = ProductionCalc::estimated_minutes(actual_prod, sample.avg_production_time);
    std::string est_duration = ProductionCalc::format_duration(est_minutes);

    // 4. 화면 출력 (FR-A-03~04)
    std::cout << "\n[ 재고 확인 ]\n"
              << "  시료    : " << sample.sample_name << "\n"
              << "  주문수량: " << order.order_quantity << " ea\n"
              << "  현재재고: " << available << " ea  (가용 재고, 선점분 제외)\n"
              << "  부족분  : " << shortage << " ea\n";

    if (shortage > 0) {
        std::cout << "  실 생산량: " << actual_prod << " ea\n"
                  << "  생산 소요: " << est_duration << "\n";
    }

    // 5. 승인/거절/취소 선택 (FR-A-05~06)
    std::cout << "\n 1. 승인  2. 거절  0. 취소\n";
    int action = InputUtil::read_int("> ", 0, 2);

    if (action == 0) return;  // 취소 → 목록으로 복귀

    if (action == 1) {
        order_approve(order);
    } else if (action == 2) {
        order_reject(order);
    }
}

// ── 승인 처리 ─────────────────────────────────────────────────────────────────

void App::order_approve(const Order& order) {
    // approved_at 기록
    std::string approved_at = Timestamp::now();

    // 가용 재고 재계산 (order_process_detail에서 이미 했지만 approve에서도 필요)
    sample_repo_.refresh();
    auto sample_opt = sample_repo_.find_by_sample_id(order.sample_id);
    if (!sample_opt.has_value()) {
        std::cout << "시료 정보를 찾을 수 없습니다.\n";
        return;
    }
    const Sample& sample = sample_opt.value();

    order_repo_.refresh();
    int64_t confirmed_sum = 0;
    bool has_producing = false;
    for (const auto& o : order_repo_.find_all()) {
        if (o.sample_id == order.sample_id && o.order_status == Order::STATUS_CONFIRMED)
            confirmed_sum += o.order_quantity;
        if (o.sample_id == order.sample_id && o.order_status == Order::STATUS_PRODUCING)
            has_producing = true;
    }
    int64_t available = std::max(int64_t(0), sample.current_stock - confirmed_sum);

    bool needs_production = (available < order.order_quantity);
    int64_t shortage = 0;
    if (needs_production) {
        shortage = (!has_producing) ? (order.order_quantity - available) : order.order_quantity;
    }

    int64_t actual_prod = ProductionCalc::actual_production(shortage, sample.yield_rate);
    double  est_minutes = ProductionCalc::estimated_minutes(actual_prod, sample.avg_production_time);
    std::string est_duration = ProductionCalc::format_duration(est_minutes);

    if (!needs_production) {
        // 재고 충분 → Confirmed 전환 (FR-A-07)
        JsonValue upd = JsonValue::object();
        upd["order_status"] = JsonValue(int64_t(Order::STATUS_CONFIRMED));
        upd["approved_at"]  = JsonValue(approved_at);
        order_store_.update(order.id, upd);
        order_repo_.refresh();

        std::cout << "처리 완료.\n"
                  << "  주문번호: " << order.order_number << "\n"
                  << "  상태    : Confirmed\n";
    } else {
        // 재고 부족 → Producing 전환 + 생산 레코드 생성 (FR-A-07)

        // 주문 상태 업데이트
        JsonValue upd = JsonValue::object();
        upd["order_status"] = JsonValue(int64_t(Order::STATUS_PRODUCING));
        upd["approved_at"]  = JsonValue(approved_at);
        order_store_.update(order.id, upd);
        order_repo_.refresh();

        // production_start_at 결정 (enqueue 규칙)
        production_repo_.refresh();

        // 현재 Producing 주문번호 집합 (방금 전환한 것 제외)
        std::set<std::string> producing_nums;
        for (const auto& o : order_repo_.find_all()) {
            if (o.order_status == Order::STATUS_PRODUCING &&
                o.order_number != order.order_number)
                producing_nums.insert(o.order_number);
        }

        // 큐 마지막 완료 시각 = max(completion_epoch)
        int64_t max_end_epoch = 0;
        for (const auto& p : production_repo_.find_all()) {
            if (producing_nums.count(p.order_number) == 0) continue;
            int64_t end_epoch = Timestamp::completion_epoch(
                p.production_start_at, p.estimated_completion);
            if (end_epoch > max_end_epoch) max_end_epoch = end_epoch;
        }

        std::string production_start_at;
        if (max_end_epoch == 0) {
            production_start_at = Timestamp::now();       // 큐 비어있음 → 즉시 시작
        } else {
            production_start_at = Timestamp::format(max_end_epoch);  // 선행 완료 시각
        }

        // 생산 레코드 저장
        JsonValue prod = JsonValue::object();
        prod["order_number"]         = JsonValue(order.order_number);
        prod["sample_name"]          = JsonValue(sample.sample_name);
        prod["order_quantity"]       = JsonValue(order.order_quantity);
        prod["shortage"]             = JsonValue(shortage);
        prod["actual_production"]    = JsonValue(actual_prod);
        prod["ordered_at"]           = JsonValue(approved_at);
        prod["estimated_completion"] = JsonValue(est_duration);
        prod["production_start_at"]  = JsonValue(production_start_at);
        production_store_.create(prod);
        production_repo_.refresh();

        std::cout << "처리 완료.\n"
                  << "  주문번호: " << order.order_number << "\n"
                  << "  상태    : Producing\n";
    }
}

// ── 거절 처리 ─────────────────────────────────────────────────────────────────

void App::order_reject(const Order& order) {
    JsonValue upd = JsonValue::object();
    upd["order_status"] = JsonValue(int64_t(Order::STATUS_REJECTED));
    order_store_.update(order.id, upd);
    order_repo_.refresh();

    std::cout << "처리 완료.\n"
              << "  주문번호: " << order.order_number << "\n"
              << "  상태    : Rejected\n";
}

void App::menu_monitoring() {
    bool loop = true;
    while (loop) {
        std::cout << "\n[ 4. 모니터링 ]\n"
                  << " 1. 주문량 확인\n"
                  << " 2. 재고량 확인\n"
                  << " 0. 돌아가기\n";
        int choice = InputUtil::read_int("> ", 0, 2);
        switch (choice) {
            case 1: monitoring_order_count(); break;
            case 2: monitoring_stock_level(); break;
            case 0: loop = false;             break;
            default: break;
        }
    }
}

void App::monitoring_order_count() {
    order_repo_.refresh();
    auto all_orders = order_repo_.find_all();

    int cnt_reserved = 0, cnt_producing = 0, cnt_confirmed = 0, cnt_released = 0;
    for (const auto& o : all_orders) {
        switch (o.order_status) {
            case Order::STATUS_RESERVED:  ++cnt_reserved;  break;
            case Order::STATUS_PRODUCING: ++cnt_producing; break;
            case Order::STATUS_CONFIRMED: ++cnt_confirmed; break;
            case Order::STATUS_RELEASED:  ++cnt_released;  break;
            default: break;
        }
    }

    std::cout << "\n[ 주문량 현황 ]\n"
              << "  Reserved  : " << cnt_reserved  << " 건\n"
              << "  Producing : " << cnt_producing << " 건\n"
              << "  Confirmed : " << cnt_confirmed << " 건\n"
              << "  Released  : " << cnt_released  << " 건\n";
}

void App::monitoring_stock_level() {
    sample_repo_.refresh();
    order_repo_.refresh();
    auto samples    = sample_repo_.find_all();
    auto all_orders = order_repo_.find_all();

    if (samples.empty()) {
        std::cout << "등록된 시료가 없습니다.\n";
        return;
    }

    TablePrinter tp({"시료명", "현재재고(ea)", "재고 상태"});
    for (const auto& s : samples) {
        bool has_producing = false;
        int64_t reserved_sum = 0, confirmed_sum = 0;
        for (const auto& o : all_orders) {
            if (o.sample_id != s.sample_id) continue;
            if (o.order_status == Order::STATUS_PRODUCING) has_producing = true;
            if (o.order_status == Order::STATUS_RESERVED)  reserved_sum  += o.order_quantity;
            if (o.order_status == Order::STATUS_CONFIRMED) confirmed_sum += o.order_quantity;
        }

        std::string status;
        if (s.current_stock == 0) {
            status = "고갈";
        } else if (!has_producing && s.current_stock >= reserved_sum + confirmed_sum) {
            status = "여유";
        } else {
            status = "부족";
        }

        tp.add_row({s.sample_name, std::to_string(s.current_stock), status});
    }
    tp.print_paged();
}

void App::menu_release_processing() {
    while (release_process_list()) {
        // 한 건 처리 후 목록 재표시
    }
}

// ── 출고 처리: Confirmed 목록 표시 + 선택 ─────────────────────────────────────
// 반환값: true=처리 완료(루프 계속), false=종료(빈 목록 또는 사용자 0 선택)

bool App::release_process_list() {
    order_repo_.refresh();
    auto confirmed = order_repo_.find_by_status(Order::STATUS_CONFIRMED);

    if (confirmed.empty()) {
        std::cout << "출고 가능한 주문이 없습니다.\n";
        return false;
    }

    // 목록 출력 (FR-R-01)
    TablePrinter tp({"No", "주문번호", "고객", "시료ID", "수량(ea)"});
    for (std::size_t i = 0; i < confirmed.size(); ++i) {
        const auto& o = confirmed[i];
        tp.add_row({
            std::to_string(i + 1),
            o.order_number,
            o.customer_name,
            o.sample_id,
            std::to_string(o.order_quantity)
        });
    }
    tp.print();

    // 번호 입력 (FR-R-02)
    int choice = InputUtil::read_int(
        "번호를 입력하세요 (0: 돌아가기): ",
        0, static_cast<int>(confirmed.size()));

    if (choice == 0) return false;

    release_process_detail(confirmed[static_cast<std::size_t>(choice - 1)]);
    return true;
}

// ── 출고 처리: 출고/취소 선택 및 처리 ────────────────────────────────────────

void App::release_process_detail(const Order& order) {
    // 주문 정보 출력
    std::cout << "\n[ 출고 확인 ]\n"
              << "  주문번호: " << order.order_number  << "\n"
              << "  고객    : " << order.customer_name << "\n"
              << "  시료 ID : " << order.sample_id     << "\n"
              << "  수량    : " << order.order_quantity << " ea\n\n"
              << " 1. 출고  0. 취소\n";

    int action = InputUtil::read_int("> ", 0, 1);
    if (action == 0) return;  // 취소 → 목록 복귀 (FR-R-04)

    // released_at 기록
    std::string released_at = Timestamp::now();

    // 주문 상태 Released 전환 + released_at 저장 (FR-R-05)
    JsonValue upd_order = JsonValue::object();
    upd_order["order_status"] = JsonValue(int64_t(Order::STATUS_RELEASED));
    upd_order["released_at"]  = JsonValue(released_at);
    order_store_.update(order.id, upd_order);
    order_repo_.refresh();

    // current_stock 차감 (FR-R-05) — 최신 파일 값 기준
    sample_repo_.refresh();
    auto sample_opt = sample_repo_.find_by_sample_id(order.sample_id);
    if (!sample_opt.has_value()) {
        std::cout << "시료 정보를 찾을 수 없습니다.\n";
        return;
    }
    const Sample& sample = sample_opt.value();
    JsonValue upd_sample = JsonValue::object();
    upd_sample["current_stock"] = JsonValue(sample.current_stock - order.order_quantity);
    sample_store_.update(sample.id, upd_sample);
    sample_repo_.refresh();

    // 처리 결과 출력 (FR-R-06)
    std::cout << "출고 처리가 완료되었습니다.\n"
              << "  주문번호: " << order.order_number  << "\n"
              << "  출고수량: " << order.order_quantity << " ea\n"
              << "  처리일시: " << released_at          << "\n"
              << "  상태    : Confirmed -> Released\n";
}

void App::menu_production_line() {
    production_line_show();
}

void App::production_line_show() {
    order_repo_.refresh();
    production_repo_.refresh();
    sample_repo_.refresh();

    // 큐 구성: Producing 주문을 approved_at 오름차순 정렬 (FR-L-04)
    std::vector<Order> producing;
    for (const auto& o : order_repo_.find_all())
        if (o.order_status == Order::STATUS_PRODUCING)
            producing.push_back(o);
    std::sort(producing.begin(), producing.end(),
        [](const Order& a, const Order& b) { return a.approved_at < b.approved_at; });

    int64_t now_epoch = Timestamp::parse(Timestamp::now());

    // ── 생산 현황 ──────────────────────────────────────────────
    std::cout << "\n[ 생산 현황 ]\n";
    if (producing.empty()) {
        std::cout << "  생산 중인 주문 없음\n";
    } else {
        const Order& front = producing[0];
        auto prod_opt   = production_repo_.find_by_order_number(front.order_number);
        auto sample_opt = sample_repo_.find_by_sample_id(front.sample_id);

        if (!prod_opt.has_value() || !sample_opt.has_value()) {
            std::cout << "  시료/생산 정보를 찾을 수 없습니다.\n";
        } else {
            const Production& prod = prod_opt.value();
            int64_t avail      = prod.order_quantity - prod.shortage;
            int     yield_pct  = static_cast<int>(sample_opt.value().yield_rate * 100);
            int64_t comp_epoch = Timestamp::completion_epoch(
                prod.production_start_at, prod.estimated_completion);
            double  pct        = Timestamp::calc_progress(
                prod.production_start_at, prod.estimated_completion, now_epoch);
            std::string comp_str = Timestamp::format_completion(comp_epoch, now_epoch);

            std::cout << "  주문번호    : " << front.order_number       << "\n"
                      << "  시료명      : " << prod.sample_name          << "\n"
                      << "  주문량      : " << prod.order_quantity       << " ea\n"
                      << "  기존 가용재고: " << avail                     << " ea\n"
                      << "  부족분      : " << prod.shortage             << " ea\n"
                      << "  실 생산량   : " << prod.actual_production    << " ea\n"
                      << "  수율        : " << yield_pct                 << " %\n"
                      << "  총생산시간  : " << prod.estimated_completion << "\n"
                      << "  진행률      : " << make_progress_bar(pct)    << "\n"
                      << "  완료 예정   : " << comp_str                  << "\n";
        }
    }

    // ── 대기 주문 ──────────────────────────────────────────────
    std::cout << "\n[ 대기 주문 ]\n";
    if (producing.size() <= 1) {
        std::cout << "  대기 중인 주문 없음\n";
    } else {
        TablePrinter tp({"순서", "주문번호", "시료명", "주문량(ea)",
                         "부족분(ea)", "실 생산량(ea)", "예상 완료시간"});
        for (std::size_t i = 1; i < producing.size(); ++i) {
            const Order& o = producing[i];
            auto p_opt = production_repo_.find_by_order_number(o.order_number);
            if (!p_opt.has_value()) continue;
            const Production& p = p_opt.value();
            int64_t comp_ep = Timestamp::completion_epoch(
                p.production_start_at, p.estimated_completion);
            std::string comp = Timestamp::format_completion(comp_ep, now_epoch);
            tp.add_row({
                std::to_string(i),
                o.order_number,
                p.sample_name,
                std::to_string(p.order_quantity),
                std::to_string(p.shortage),
                std::to_string(p.actual_production),
                comp
            });
        }
        tp.print_paged();
    }

    std::cout << "\n계속하려면 Enter를 누르세요...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

std::string App::make_progress_bar(double pct, int width) {
    int filled = static_cast<int>(pct / 100.0 * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
        bar += (i < filled) ? u8"█" : u8"░";
    bar += "] ";
    bar += std::to_string(static_cast<int>(pct));
    bar += " %";
    return bar;
}

void App::evaluate_producing_orders() {
    order_repo_.refresh();
    production_repo_.refresh();

    int64_t now_epoch = Timestamp::parse(Timestamp::now());

    struct QueueEntry { Order order; Production prod; };
    std::vector<QueueEntry> queue;

    for (const auto& o : order_repo_.find_all()) {
        if (o.order_status != Order::STATUS_PRODUCING) continue;
        auto p_opt = production_repo_.find_by_order_number(o.order_number);
        if (!p_opt.has_value()) continue;
        queue.push_back({o, p_opt.value()});
    }

    std::sort(queue.begin(), queue.end(),
        [](const QueueEntry& a, const QueueEntry& b) {
            return a.prod.production_start_at < b.prod.production_start_at;
        });

    for (const auto& entry : queue) {
        int64_t comp_epoch = Timestamp::completion_epoch(
            entry.prod.production_start_at, entry.prod.estimated_completion);

        if (comp_epoch > now_epoch) continue;

        // Producing → Confirmed 전환
        JsonValue upd_order = JsonValue::object();
        upd_order["order_status"] = JsonValue(int64_t(Order::STATUS_CONFIRMED));
        order_store_.update(entry.order.id, upd_order);

        // current_stock += actual_production (루프마다 최신 재고 재조회)
        sample_repo_.refresh();
        auto sample_opt = sample_repo_.find_by_sample_id(entry.order.sample_id);
        if (sample_opt.has_value()) {
            const Sample& s = sample_opt.value();
            JsonValue upd_sample = JsonValue::object();
            upd_sample["current_stock"] = JsonValue(
                s.current_stock + entry.prod.actual_production);
            sample_store_.update(s.id, upd_sample);
        }

        std::cout << "[자동 전환] " << entry.order.order_number
                  << ": Producing -> Confirmed"
                  << " (재고 +" << entry.prod.actual_production << " ea)\n";
    }

    order_repo_.refresh();
    sample_repo_.refresh();
}

} // namespace mvc
