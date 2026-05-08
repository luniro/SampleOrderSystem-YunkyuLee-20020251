// tests/mvc/test_monitoring.cpp — TG-MN: Phase 5 모니터링 테스트
#include <gtest/gtest.h>
#include "mvc/App.hpp"
#include "data_store.hpp"
#include "monitor/repository/sample_repository.hpp"
#include "monitor/repository/order_repository.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string run_app_with_input(const std::string& input,
                                      const std::string& data_dir) {
    std::istringstream in_stream(input);
    std::streambuf* old_cin = std::cin.rdbuf(in_stream.rdbuf());

    std::ostringstream out_stream;
    std::streambuf* old_cout = std::cout.rdbuf(out_stream.rdbuf());

    mvc::AppConfig config;
    config.data_dir = data_dir;
    mvc::App app(config);
    app.run();

    std::cin.rdbuf(old_cin);
    std::cout.rdbuf(old_cout);

    return out_stream.str();
}

static std::string make_temp_dir(const std::string& suffix) {
    auto tmp = fs::temp_directory_path() / ("mn_test_" + suffix);
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp);
    return tmp.string();
}

static void remove_dir(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

// Helper: 시료 등록 input 시퀀스 생성
// menu 1 → sub 1 → sid, name, apt, yr, stock → sub 0
static std::string sample_register_input(
    const std::string& sid, const std::string& name,
    const std::string& apt, const std::string& yr, const std::string& stock)
{
    return "1\n1\n" + sid + "\n" + name + "\n" + apt + "\n" + yr + "\n" + stock + "\n0\n";
}

// Helper: 주문 접수 input 시퀀스 생성
static std::string order_reception_input(
    const std::string& sid, const std::string& customer,
    const std::string& qty)
{
    return "2\n" + sid + "\n" + customer + "\n" + qty + "\n1\n";
}

// Helper: DataStore에 직접 주문 레코드 삽입 (test fixture)
static void insert_order_direct(const std::string& data_dir,
                                 const std::string& order_number,
                                 const std::string& sample_id,
                                 int64_t qty,
                                 int status)
{
    DataStore ds((fs::path(data_dir) / "orders.json").string());
    JsonValue rec = JsonValue::object();
    rec["order_number"]   = JsonValue(order_number);
    rec["sample_id"]      = JsonValue(sample_id);
    rec["customer_name"]  = JsonValue(std::string("테스트고객"));
    rec["order_quantity"] = JsonValue(qty);
    rec["order_status"]   = JsonValue(int64_t(status));
    rec["approved_at"]    = JsonValue(nullptr);
    rec["released_at"]    = JsonValue(nullptr);
    ds.create(rec);
}

// Helper: DataStore에 직접 시료 레코드 삽입 (test fixture)
static void insert_sample_direct(const std::string& data_dir,
                                  const std::string& sample_id,
                                  const std::string& sample_name,
                                  int64_t current_stock)
{
    DataStore ds((fs::path(data_dir) / "samples.json").string());
    JsonValue rec = JsonValue::object();
    rec["sample_id"]           = JsonValue(sample_id);
    rec["sample_name"]         = JsonValue(sample_name);
    rec["avg_production_time"] = JsonValue(2.0);
    rec["yield_rate"]          = JsonValue(0.85);
    rec["current_stock"]       = JsonValue(current_stock);
    ds.create(rec);
}

// ── TC-MN-01: 주문 없을 때 상태별 건수 모두 0 출력 ───────────────────────────
TEST(TG_MN, TC_MN_01_NoOrders_AllZero) {
    auto dir = make_temp_dir("01");

    // 메인 4 → 서브 1(주문량 확인) → 0(돌아가기) → 0(종료)
    std::string input = "4\n1\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("Reserved  : 0 건"), std::string::npos)
        << "Reserved 0건 미출력\n" << output;
    EXPECT_NE(output.find("Producing : 0 건"), std::string::npos)
        << "Producing 0건 미출력\n" << output;
    EXPECT_NE(output.find("Confirmed : 0 건"), std::string::npos)
        << "Confirmed 0건 미출력\n" << output;
    EXPECT_NE(output.find("Released  : 0 건"), std::string::npos)
        << "Released 0건 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-MN-02: 각 상태별 주문 건수 정확히 카운팅, Rejected 제외 ──────────────
TEST(TG_MN, TC_MN_02_OrderCounts_RejectExcluded) {
    auto dir = make_temp_dir("02");

    // 시료 등록
    insert_sample_direct(dir, "S-001", "테스트시료", 1000);

    // 주문 삽입: Reserved×2, Producing×1, Confirmed×1, Released×1, Rejected×1
    insert_order_direct(dir, "ORD-01", "S-001", 10, Order::STATUS_RESERVED);
    insert_order_direct(dir, "ORD-02", "S-001", 10, Order::STATUS_RESERVED);
    insert_order_direct(dir, "ORD-03", "S-001", 10, Order::STATUS_PRODUCING);
    insert_order_direct(dir, "ORD-04", "S-001", 10, Order::STATUS_CONFIRMED);
    insert_order_direct(dir, "ORD-05", "S-001", 10, Order::STATUS_RELEASED);
    insert_order_direct(dir, "ORD-06", "S-001", 10, Order::STATUS_REJECTED);

    std::string input = "4\n1\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("Reserved  : 2 건"), std::string::npos)
        << "Reserved 2건 미출력\n" << output;
    EXPECT_NE(output.find("Producing : 1 건"), std::string::npos)
        << "Producing 1건 미출력\n" << output;
    EXPECT_NE(output.find("Confirmed : 1 건"), std::string::npos)
        << "Confirmed 1건 미출력\n" << output;
    EXPECT_NE(output.find("Released  : 1 건"), std::string::npos)
        << "Released 1건 미출력\n" << output;
    // Rejected는 출력되지 않음
    EXPECT_EQ(output.find("Rejected  :"), std::string::npos)
        << "Rejected 항목이 출력되면 안 됨\n" << output;

    remove_dir(dir);
}

// ── TC-MN-03: 시료 없을 때 "등록된 시료가 없습니다." 출력 ─────────────────────
TEST(TG_MN, TC_MN_03_NoSamples_InfoMessage) {
    auto dir = make_temp_dir("03");

    // 시료 미등록 상태에서 재고량 확인
    std::string input = "4\n2\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("등록된 시료가 없습니다"), std::string::npos)
        << "시료 없음 안내 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-MN-04: current_stock=0 → 재고 상태 "고갈" ────────────────────────────
TEST(TG_MN, TC_MN_04_StockZero_Depleted) {
    auto dir = make_temp_dir("04");

    insert_sample_direct(dir, "S-004", "고갈시료", 0);

    std::string input = "4\n2\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("고갈"), std::string::npos)
        << "고갈 상태 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-MN-05: current_stock>0, Producing 없음, stock >= Reserved합+Confirmed합 → "여유" ──
TEST(TG_MN, TC_MN_05_StockSufficient_Surplus) {
    auto dir = make_temp_dir("05");

    // 재고 100, Reserved 30 + Confirmed 20 = 50 <= 100 → 여유
    insert_sample_direct(dir, "S-005", "여유시료", 100);
    insert_order_direct(dir, "ORD-05A", "S-005", 30, Order::STATUS_RESERVED);
    insert_order_direct(dir, "ORD-05B", "S-005", 20, Order::STATUS_CONFIRMED);

    std::string input = "4\n2\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("여유"), std::string::npos)
        << "여유 상태 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-MN-06: current_stock>0, Producing 있음 → "부족" ──────────────────────
TEST(TG_MN, TC_MN_06_HasProducing_Shortage) {
    auto dir = make_temp_dir("06");

    // 재고 100이더라도 Producing 주문이 있으면 부족
    insert_sample_direct(dir, "S-006", "부족시료A", 100);
    insert_order_direct(dir, "ORD-06A", "S-006", 50, Order::STATUS_PRODUCING);

    std::string input = "4\n2\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("부족"), std::string::npos)
        << "부족 상태 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-MN-07: current_stock>0, Producing 없음, stock < Reserved합+Confirmed합 → "부족" ──
TEST(TG_MN, TC_MN_07_StockInsufficient_Shortage) {
    auto dir = make_temp_dir("07");

    // 재고 40, Reserved 30 + Confirmed 20 = 50 > 40 → 부족
    insert_sample_direct(dir, "S-007", "부족시료B", 40);
    insert_order_direct(dir, "ORD-07A", "S-007", 30, Order::STATUS_RESERVED);
    insert_order_direct(dir, "ORD-07B", "S-007", 20, Order::STATUS_CONFIRMED);

    std::string input = "4\n2\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("부족"), std::string::npos)
        << "부족 상태 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-MN-08: current_stock=0이면서 Producing도 있는 경우 → 우선순위 "고갈" ───
TEST(TG_MN, TC_MN_08_StockZeroWithProducing_Depleted) {
    auto dir = make_temp_dir("08");

    // 재고 0 + Producing 주문 있음 → 고갈 우선
    insert_sample_direct(dir, "S-008", "우선순위시료", 0);
    insert_order_direct(dir, "ORD-08A", "S-008", 30, Order::STATUS_PRODUCING);
    insert_order_direct(dir, "ORD-08B", "S-008", 10, Order::STATUS_RESERVED);

    std::string input = "4\n2\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("고갈"), std::string::npos)
        << "고갈 우선순위 미적용\n" << output;
    // 부족이 출력되면 안 됨 (고갈이 먼저)
    // 테이블에 "고갈"이 포함되어야 함

    remove_dir(dir);
}

// ── TC-MN-09: 여러 시료 혼재 시 시료별 독립 판정 확인 ───────────────────────
TEST(TG_MN, TC_MN_09_MultipleSamples_IndependentStatus) {
    auto dir = make_temp_dir("09");

    // 시료1: 재고 0 → 고갈
    insert_sample_direct(dir, "S-009A", "고갈시료A", 0);
    // 시료2: 재고 100, Producing 없음, Reserved 30 + Confirmed 10 = 40 <= 100 → 여유
    insert_sample_direct(dir, "S-009B", "여유시료B", 100);
    // 시료3: 재고 10, Producing 있음 → 부족
    insert_sample_direct(dir, "S-009C", "부족시료C", 10);

    insert_order_direct(dir, "ORD-09A", "S-009B", 30, Order::STATUS_RESERVED);
    insert_order_direct(dir, "ORD-09B", "S-009B", 10, Order::STATUS_CONFIRMED);
    insert_order_direct(dir, "ORD-09C", "S-009C", 20, Order::STATUS_PRODUCING);

    std::string input = "4\n2\n0\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("고갈"), std::string::npos)
        << "고갈 시료 미출력\n" << output;
    EXPECT_NE(output.find("여유"), std::string::npos)
        << "여유 시료 미출력\n" << output;
    EXPECT_NE(output.find("부족"), std::string::npos)
        << "부족 시료 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-MN-10: menu_monitoring() 진입마다 파일 최신 데이터 반영(refresh 호출 여부) ──
TEST(TG_MN, TC_MN_10_RefreshOnEntry) {
    auto dir = make_temp_dir("10");

    // 1단계: 시료 등록 + 첫 주문량 확인 (주문 없음 → 모두 0건)
    std::string input1 =
        sample_register_input("S-010", "갱신시료", "2.0", "0.80", "200") +
        "4\n1\n0\n0\n";
    run_app_with_input(input1, dir);

    // 2단계: 주문 접수 (Reserved 1건 생성)
    std::string input2 =
        order_reception_input("S-010", "고객갱신", "50") +
        "0\n";
    run_app_with_input(input2, dir);

    // 3단계: 재진입하여 주문량 확인 → Reserved 1건이 반영되어야 함
    std::string input3 = "4\n1\n0\n0\n";
    std::string output3 = run_app_with_input(input3, dir);

    EXPECT_NE(output3.find("Reserved  : 1 건"), std::string::npos)
        << "refresh 후 Reserved 1건 미반영\n" << output3;

    remove_dir(dir);
}
