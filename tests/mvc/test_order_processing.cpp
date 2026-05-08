// tests/mvc/test_order_processing.cpp — TG-AP: Phase 4 주문 처리 테스트
#include <gtest/gtest.h>
#include "mvc/App.hpp"
#include "data_store.hpp"
#include "monitor/repository/order_repository.hpp"
#include "monitor/repository/production_repository.hpp"
#include "monitor/util/timestamp.hpp"

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
    auto tmp = fs::temp_directory_path() / ("ap_test_" + suffix);
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
// sample_id, sample_name, avg_production_time, yield_rate, current_stock
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

// ── TC-AP-01: Reserved 주문 없을 때 안내 메시지 출력 ──────────────────────────
TEST(TG_AP, TC_AP_01_NoReservedOrders_InfoMessage) {
    auto dir = make_temp_dir("01");

    // 주문 없이 메뉴 3 진입
    std::string input =
        "3\n"   // 주문 처리 메뉴
        "0\n";  // 종료

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("처리 대기 중인 주문이 없습니다"), std::string::npos)
        << "빈 목록 안내 메시지 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-AP-02: 재고 충분(shortage=0) → 승인 → Confirmed 전환 확인 ─────────────
TEST(TG_AP, TC_AP_02_ApproveWithSufficientStock_Confirmed) {
    auto dir = make_temp_dir("02");

    // 시료 등록 (재고 100), 주문 접수 (수량 50), 주문 처리 승인
    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "100") +
        order_reception_input("S-001", "홍길동", "50") +
        "3\n"   // 주문 처리 메뉴
        "1\n"   // 첫 번째 주문 선택
        "1\n"   // 승인
        "0\n";  // 종료

    std::string output = run_app_with_input(input, dir);

    // orders.json 확인
    std::string orders_path = (fs::path(dir) / "orders.json").string();
    ASSERT_TRUE(fs::exists(orders_path));
    DataStore ds(orders_path);
    auto records = ds.read_all();
    ASSERT_EQ(static_cast<int>(records.size()), 1);
    EXPECT_EQ(records[0]["order_status"].as_integer(), int64_t(Order::STATUS_CONFIRMED))
        << "order_status가 Confirmed(3)이어야 함";

    // 출력 확인
    EXPECT_NE(output.find("Confirmed"), std::string::npos)
        << "Confirmed 상태 출력 미발견\n" << output;
    EXPECT_NE(output.find("처리 완료"), std::string::npos)
        << "처리 완료 메시지 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-AP-03: 재고 부족, Producing 없음(Case 1) → 승인 → Producing + 생산 레코드 ─
TEST(TG_AP, TC_AP_03_ApproveWithShortage_Case1_Producing) {
    auto dir = make_temp_dir("03");

    // 재고 30, 주문 50 → 부족분 = 50 - 30 = 20
    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "30") +
        order_reception_input("S-001", "홍길동", "50") +
        "3\n"   // 주문 처리
        "1\n"   // 첫 번째 주문 선택
        "1\n"   // 승인
        "0\n";  // 종료

    std::string output = run_app_with_input(input, dir);

    // 주문 상태가 Producing 인지 확인
    std::string orders_path = (fs::path(dir) / "orders.json").string();
    ASSERT_TRUE(fs::exists(orders_path));
    DataStore ds_ord(orders_path);
    auto ord_records = ds_ord.read_all();
    ASSERT_EQ(static_cast<int>(ord_records.size()), 1);
    EXPECT_EQ(ord_records[0]["order_status"].as_integer(), int64_t(Order::STATUS_PRODUCING))
        << "order_status가 Producing(2)이어야 함";

    // 생산 레코드 생성 확인
    std::string prod_path = (fs::path(dir) / "productions.json").string();
    ASSERT_TRUE(fs::exists(prod_path)) << "productions.json이 생성되어야 함";
    DataStore ds_prod(prod_path);
    auto prod_records = ds_prod.read_all();
    ASSERT_EQ(static_cast<int>(prod_records.size()), 1);
    EXPECT_EQ(prod_records[0]["shortage"].as_integer(), int64_t(20))
        << "shortage가 20(=50-30)이어야 함 (Case 1)";

    // 출력 확인
    EXPECT_NE(output.find("Producing"), std::string::npos)
        << "Producing 상태 출력 미발견\n" << output;

    remove_dir(dir);
}

// ── TC-AP-04: 재고 부족, Producing 이미 존재(Case 2) → shortage = order_quantity ─
TEST(TG_AP, TC_AP_04_ApproveWithShortage_Case2_FullShortage) {
    auto dir = make_temp_dir("04");

    // 재고 30, 첫 주문 50 → Case 1(부족분=20), 두 번째 주문 40 → Case 2(부족분=40)
    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "30") +
        order_reception_input("S-001", "홍길동", "50") +
        order_reception_input("S-001", "김철수", "40") +
        "3\n"   // 주문 처리
        "1\n"   // 첫 번째 주문 선택
        "1\n"   // 승인 → Producing (Case 1, shortage=20)
        "1\n"   // 두 번째 주문 선택
        "1\n"   // 승인 → Producing (Case 2, shortage=40)
        "0\n";  // 종료

    run_app_with_input(input, dir);

    // 생산 레코드 확인
    std::string prod_path = (fs::path(dir) / "productions.json").string();
    ASSERT_TRUE(fs::exists(prod_path));
    DataStore ds_prod(prod_path);
    auto prod_records = ds_prod.read_all();
    ASSERT_EQ(static_cast<int>(prod_records.size()), 2);

    // 두 번째 생산 레코드의 shortage가 order_quantity(40)와 동일해야 함
    // 레코드 순서는 삽입 순서이므로 [1]이 두 번째
    EXPECT_EQ(prod_records[1]["shortage"].as_integer(), int64_t(40))
        << "Case 2: shortage가 order_quantity(40)와 동일해야 함";

    remove_dir(dir);
}

// ── TC-AP-05: 거절 → Rejected 전환 확인 ─────────────────────────────────────
TEST(TG_AP, TC_AP_05_Reject_StatusBecomeRejected) {
    auto dir = make_temp_dir("05");

    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "100") +
        order_reception_input("S-001", "홍길동", "50") +
        "3\n"   // 주문 처리
        "1\n"   // 첫 번째 주문 선택
        "2\n"   // 거절
        "0\n";  // 종료

    std::string output = run_app_with_input(input, dir);

    // orders.json 확인
    std::string orders_path = (fs::path(dir) / "orders.json").string();
    ASSERT_TRUE(fs::exists(orders_path));
    DataStore ds(orders_path);
    auto records = ds.read_all();
    ASSERT_EQ(static_cast<int>(records.size()), 1);
    EXPECT_EQ(records[0]["order_status"].as_integer(), int64_t(Order::STATUS_REJECTED))
        << "order_status가 Rejected(1)이어야 함";

    // 출력 확인
    EXPECT_NE(output.find("Rejected"), std::string::npos)
        << "Rejected 상태 출력 미발견\n" << output;
    EXPECT_NE(output.find("처리 완료"), std::string::npos)
        << "처리 완료 메시지 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-AP-06: 취소(0) → 주문 상태 미변경 확인 ───────────────────────────────
TEST(TG_AP, TC_AP_06_Cancel_StatusUnchanged) {
    auto dir = make_temp_dir("06");

    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "100") +
        order_reception_input("S-001", "홍길동", "50") +
        "3\n"   // 주문 처리
        "1\n"   // 첫 번째 주문 선택
        "0\n"   // 취소
        "0\n"   // 목록에서 돌아가기
        "0\n";  // 종료

    run_app_with_input(input, dir);

    // orders.json 확인 — 여전히 Reserved
    std::string orders_path = (fs::path(dir) / "orders.json").string();
    ASSERT_TRUE(fs::exists(orders_path));
    DataStore ds(orders_path);
    auto records = ds.read_all();
    ASSERT_EQ(static_cast<int>(records.size()), 1);
    EXPECT_EQ(records[0]["order_status"].as_integer(), int64_t(Order::STATUS_RESERVED))
        << "취소 후 order_status가 Reserved(0)으로 유지되어야 함";

    remove_dir(dir);
}

// ── TC-AP-07: approved_at 기록 확인 (승인 시) ────────────────────────────────
TEST(TG_AP, TC_AP_07_ApprovedAt_RecordedOnApproval) {
    auto dir = make_temp_dir("07");

    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "100") +
        order_reception_input("S-001", "홍길동", "50") +
        "3\n"
        "1\n"
        "1\n"   // 승인
        "0\n";

    run_app_with_input(input, dir);

    std::string orders_path = (fs::path(dir) / "orders.json").string();
    ASSERT_TRUE(fs::exists(orders_path));
    DataStore ds(orders_path);
    auto records = ds.read_all();
    ASSERT_EQ(static_cast<int>(records.size()), 1);

    // approved_at이 null이 아니고 "YYYY-MM-DD HH:MM:SS" 형식이어야 함
    ASSERT_FALSE(records[0]["approved_at"].is_null())
        << "승인 후 approved_at이 null이 아니어야 함";
    std::string at = records[0]["approved_at"].as_string();
    EXPECT_EQ(at.size(), std::size_t(19))
        << "approved_at 형식이 YYYY-MM-DD HH:MM:SS이어야 함. 값: " << at;
    EXPECT_EQ(at[4], '-');
    EXPECT_EQ(at[7], '-');
    EXPECT_EQ(at[10], ' ');
    EXPECT_EQ(at[13], ':');
    EXPECT_EQ(at[16], ':');

    remove_dir(dir);
}

// ── TC-AP-08: production_start_at — 큐 빈 경우 = approved_at ─────────────────
TEST(TG_AP, TC_AP_08_ProductionStartAt_EmptyQueue_EqualsApprovedAt) {
    auto dir = make_temp_dir("08");

    // 재고 10, 주문 50 → shortage=40 → Producing
    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "10") +
        order_reception_input("S-001", "홍길동", "50") +
        "3\n"
        "1\n"
        "1\n"   // 승인 → Producing (큐 비어있음)
        "0\n";

    run_app_with_input(input, dir);

    std::string orders_path = (fs::path(dir) / "orders.json").string();
    std::string prod_path   = (fs::path(dir) / "productions.json").string();

    ASSERT_TRUE(fs::exists(orders_path));
    ASSERT_TRUE(fs::exists(prod_path));

    DataStore ds_ord(orders_path);
    auto ord_recs = ds_ord.read_all();
    ASSERT_EQ(static_cast<int>(ord_recs.size()), 1);
    std::string approved_at = ord_recs[0]["approved_at"].as_string();

    DataStore ds_prod(prod_path);
    auto prod_recs = ds_prod.read_all();
    ASSERT_EQ(static_cast<int>(prod_recs.size()), 1);
    std::string prod_start = prod_recs[0]["production_start_at"].as_string();

    // 큐가 비어있으므로 production_start_at == approved_at
    EXPECT_EQ(prod_start, approved_at)
        << "큐 빈 경우 production_start_at이 approved_at과 같아야 함";

    remove_dir(dir);
}

// ── TC-AP-09: production_start_at — 선행 Producing 있을 경우 = 선행 완료 시각 ──
TEST(TG_AP, TC_AP_09_ProductionStartAt_WithPriorProducing_EqualsQueueEnd) {
    auto dir = make_temp_dir("09");

    // 재고 0, 첫 주문(shortage=50), 두 번째 주문(shortage=40)
    // 두 번째 주문의 production_start_at = 첫 주문의 production_start_at + estimated_completion
    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "0.5", "0.90", "0") +
        order_reception_input("S-001", "홍길동", "50") +
        order_reception_input("S-001", "김철수", "40") +
        "3\n"
        "1\n"   // 첫 번째 주문
        "1\n"   // 승인 (큐 비어있음, start=approved_at)
        "1\n"   // 두 번째 주문
        "1\n"   // 승인 (선행 있음)
        "0\n";

    run_app_with_input(input, dir);

    std::string prod_path = (fs::path(dir) / "productions.json").string();
    ASSERT_TRUE(fs::exists(prod_path));

    DataStore ds_prod(prod_path);
    auto prod_recs = ds_prod.read_all();
    ASSERT_EQ(static_cast<int>(prod_recs.size()), 2);

    // 첫 번째 레코드: production_start_at + estimated_completion = 완료 시각
    std::string first_start  = prod_recs[0]["production_start_at"].as_string();
    std::string first_est    = prod_recs[0]["estimated_completion"].as_string();
    std::string second_start = prod_recs[1]["production_start_at"].as_string();

    // 두 번째 production_start_at >= 첫 번째 완료 시각
    // (동일하거나 이후여야 함)
    int64_t first_end  = Timestamp::completion_epoch(first_start, first_est);
    int64_t second_ep  = Timestamp::parse(second_start);

    EXPECT_EQ(second_ep, first_end)
        << "두 번째 production_start_at이 첫 번째 완료 시각이어야 함"
        << "\nfirst_start=" << first_start << " first_est=" << first_est
        << "\nfirst_end=" << Timestamp::format(first_end)
        << "\nsecond_start=" << second_start;

    remove_dir(dir);
}

// ── TC-AP-10: 가용 재고 계산 — Confirmed 선점분 차감 확인 ──────────────────────
TEST(TG_AP, TC_AP_10_AvailableStock_ConfirmedDeducted) {
    auto dir = make_temp_dir("10");

    // 재고 100, 첫 주문 80 → Confirmed(80 선점), 두 번째 주문 30
    // 가용 재고 = 100 - 80 = 20 < 30 → Producing, shortage = 30 - 20 = 10
    std::string input =
        sample_register_input("S-001", "실리콘웨이퍼", "1.5", "0.90", "100") +
        order_reception_input("S-001", "홍길동", "80") +
        order_reception_input("S-001", "김철수", "30") +
        "3\n"
        "1\n"   // 첫 번째 주문
        "1\n"   // 승인 → Confirmed (재고 충분: 100>=80)
        "1\n"   // 두 번째 주문
        "1\n"   // 승인 → 가용=100-80=20 < 30 → Producing
        "0\n";

    std::string output = run_app_with_input(input, dir);

    // 두 번째 주문 처리 후 Producing 상태 확인
    std::string prod_path = (fs::path(dir) / "productions.json").string();
    ASSERT_TRUE(fs::exists(prod_path));
    DataStore ds_prod(prod_path);
    auto prod_recs = ds_prod.read_all();
    ASSERT_EQ(static_cast<int>(prod_recs.size()), 1);

    // shortage = 30 - 20 = 10 (Case 1: 첫 Producing)
    EXPECT_EQ(prod_recs[0]["shortage"].as_integer(), int64_t(10))
        << "Confirmed 선점 후 가용 재고 기반 shortage 계산 오류";

    // 출력에서 재고 확인 화면 내 현재재고 20 ea 확인
    EXPECT_NE(output.find("20 ea"), std::string::npos)
        << "가용 재고 20 ea가 화면에 출력되어야 함\n" << output;

    remove_dir(dir);
}
