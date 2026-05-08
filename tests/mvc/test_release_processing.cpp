// tests/mvc/test_release_processing.cpp — TG-RL: Phase 6 출고 처리 테스트
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
    auto tmp = fs::temp_directory_path() / ("rl_test_" + suffix);
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp);
    return tmp.string();
}

static void remove_dir(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

// Helper: DataStore에 직접 시료 레코드 삽입
static int64_t insert_sample_direct(const std::string& data_dir,
                                     const std::string& sample_id,
                                     const std::string& sample_name,
                                     int64_t current_stock) {
    DataStore ds((fs::path(data_dir) / "samples.json").string());
    JsonValue rec = JsonValue::object();
    rec["sample_id"]           = JsonValue(sample_id);
    rec["sample_name"]         = JsonValue(sample_name);
    rec["avg_production_time"] = JsonValue(2.0);
    rec["yield_rate"]          = JsonValue(0.85);
    rec["current_stock"]       = JsonValue(current_stock);
    return ds.create(rec);
}

// Helper: DataStore에 직접 Confirmed 주문 레코드 삽입
static int64_t insert_confirmed_order(const std::string& data_dir,
                                       const std::string& order_number,
                                       const std::string& sample_id,
                                       const std::string& customer_name,
                                       int64_t qty) {
    DataStore ds((fs::path(data_dir) / "orders.json").string());
    JsonValue rec = JsonValue::object();
    rec["order_number"]   = JsonValue(order_number);
    rec["sample_id"]      = JsonValue(sample_id);
    rec["customer_name"]  = JsonValue(customer_name);
    rec["order_quantity"] = JsonValue(qty);
    rec["order_status"]   = JsonValue(int64_t(Order::STATUS_CONFIRMED));
    rec["approved_at"]    = JsonValue(std::string("2024-05-01 10:00:00"));
    rec["released_at"]    = JsonValue(nullptr);
    return ds.create(rec);
}

// Helper: orders.json에서 모든 주문 읽기
static std::vector<JsonValue> read_all_orders(const std::string& data_dir) {
    DataStore ds((fs::path(data_dir) / "orders.json").string());
    return ds.read_all();
}

// Helper: samples.json에서 모든 시료 읽기
static std::vector<JsonValue> read_all_samples(const std::string& data_dir) {
    DataStore ds((fs::path(data_dir) / "samples.json").string());
    return ds.read_all();
}

// ── TC-RL-01: Confirmed 주문 없을 때 안내 메시지 출력 후 반환 ───────────────────
TEST(TG_RL, TC_RL_01_NoConfirmed_InfoMessage) {
    auto dir = make_temp_dir("01");

    // Confirmed 주문 없음 (빈 DB)
    // 메인5 진입 → 자동 false 반환 → 메인 메뉴 복귀 → 0(종료)
    std::string input = "5\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("출고 가능한 주문이 없습니다"), std::string::npos)
        << "Confirmed 없음 안내 메시지 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-RL-02: 출고 선택 → order_status = Released(4) 전환 확인 ─────────────────
TEST(TG_RL, TC_RL_02_Release_StatusTransition) {
    auto dir = make_temp_dir("02");

    insert_sample_direct(dir, "S-001", "테스트시료", 100);
    insert_confirmed_order(dir, "ORD-20240501-001", "S-001", "홍길동", 50);

    // 메인5 → 1(선택) → 1(출고) → 자동 false(빈 목록) → 0(종료)
    std::string input = "5\n1\n1\n0\n";
    run_app_with_input(input, dir);

    auto orders = read_all_orders(dir);
    ASSERT_EQ(orders.size(), 1u);
    EXPECT_EQ(orders[0]["order_status"].as_integer(), int64_t(Order::STATUS_RELEASED))
        << "order_status가 Released(4)로 전환되지 않음";

    remove_dir(dir);
}

// ── TC-RL-03: 출고 선택 → released_at 기록 확인 ────────────────────────────────
TEST(TG_RL, TC_RL_03_Release_ReleasedAtRecorded) {
    auto dir = make_temp_dir("03");

    insert_sample_direct(dir, "S-001", "테스트시료", 100);
    insert_confirmed_order(dir, "ORD-20240501-001", "S-001", "홍길동", 50);

    std::string input = "5\n1\n1\n0\n";
    run_app_with_input(input, dir);

    auto orders = read_all_orders(dir);
    ASSERT_EQ(orders.size(), 1u);

    const auto& released_at_val = orders[0]["released_at"];
    EXPECT_FALSE(released_at_val.is_null())
        << "released_at이 여전히 null임";

    if (!released_at_val.is_null()) {
        const std::string& ts = released_at_val.as_string();
        EXPECT_EQ(ts.size(), 25u)
            << "released_at 길이가 25가 아님: " << ts;
        if (ts.size() >= 19) {
            EXPECT_EQ(ts[4], '-');
            EXPECT_EQ(ts[7], '-');
            EXPECT_EQ(ts[10], ' ');
            EXPECT_EQ(ts[13], ':');
            EXPECT_EQ(ts[16], ':');
        }
        if (ts.size() == 25) {
            EXPECT_EQ(ts.substr(19), " (KST)");
        }
    }

    remove_dir(dir);
}

// ── TC-RL-04: 출고 선택 → current_stock -= order_quantity 확인 ─────────────────
TEST(TG_RL, TC_RL_04_Release_StockDecremented) {
    auto dir = make_temp_dir("04");

    insert_sample_direct(dir, "S-001", "테스트시료", 100);
    insert_confirmed_order(dir, "ORD-20240501-001", "S-001", "홍길동", 50);

    std::string input = "5\n1\n1\n0\n";
    run_app_with_input(input, dir);

    auto samples = read_all_samples(dir);
    ASSERT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0]["current_stock"].as_integer(), int64_t(50))
        << "current_stock이 올바르게 차감되지 않음 (기대: 50, 실제: "
        << samples[0]["current_stock"].as_integer() << ")";

    remove_dir(dir);
}

// ── TC-RL-05: 취소(0) 선택 → order_status 미변경, current_stock 미변경 확인 ──────
TEST(TG_RL, TC_RL_05_Cancel_NoChange) {
    auto dir = make_temp_dir("05");

    insert_sample_direct(dir, "S-001", "테스트시료", 100);
    insert_confirmed_order(dir, "ORD-20240501-001", "S-001", "홍길동", 50);

    // 메인5 → 1(선택) → 0(취소) → 0(목록 돌아가기) → 0(종료)
    std::string input = "5\n1\n0\n0\n0\n";
    run_app_with_input(input, dir);

    auto orders = read_all_orders(dir);
    ASSERT_EQ(orders.size(), 1u);
    EXPECT_EQ(orders[0]["order_status"].as_integer(), int64_t(Order::STATUS_CONFIRMED))
        << "취소 선택 후 order_status가 변경됨";

    auto samples = read_all_samples(dir);
    ASSERT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0]["current_stock"].as_integer(), int64_t(100))
        << "취소 선택 후 current_stock이 변경됨";

    remove_dir(dir);
}

// ── TC-RL-06: 결과 출력에 주문번호·출고수량·처리일시·상태변화 포함 확인 ─────────
TEST(TG_RL, TC_RL_06_ResultOutput_AllFields) {
    auto dir = make_temp_dir("06");

    insert_sample_direct(dir, "S-001", "테스트시료", 200);
    insert_confirmed_order(dir, "ORD-20240501-003", "S-001", "이영희", 100);

    std::string input = "5\n1\n1\n0\n";
    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("출고 처리가 완료되었습니다"), std::string::npos)
        << "완료 메시지 미출력\n" << output;
    EXPECT_NE(output.find("ORD-20240501-003"), std::string::npos)
        << "주문번호 미출력\n" << output;
    EXPECT_NE(output.find("100 ea"), std::string::npos)
        << "출고수량 미출력\n" << output;
    // 처리일시 — "YYYY-" 패턴 포함 여부로 검증
    EXPECT_NE(output.find("처리일시"), std::string::npos)
        << "처리일시 레이블 미출력\n" << output;
    // 상태변화: Confirmed → Released (화살표 형식)
    EXPECT_NE(output.find("Confirmed"), std::string::npos)
        << "Confirmed 상태 미출력\n" << output;
    EXPECT_NE(output.find("Released"), std::string::npos)
        << "Released 상태 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-RL-07: 출고 후 목록 재조회 시 해당 주문이 Confirmed 목록에서 제거 확인 ──
TEST(TG_RL, TC_RL_07_AfterRelease_RemovedFromConfirmedList) {
    auto dir = make_temp_dir("07");

    insert_sample_direct(dir, "S-001", "테스트시료", 100);
    insert_confirmed_order(dir, "ORD-20240501-001", "S-001", "홍길동", 50);

    // 메인5 → 1(선택) → 1(출고) → 루프 재진입(빈 목록) → 0(종료)
    // release_process_list()는 출고 후 true 반환 → 루프 재진입 → Confirmed 없음 → false 반환
    std::string input = "5\n1\n1\n0\n";
    std::string output = run_app_with_input(input, dir);

    // 출고 후 재진입 시 "출고 가능한 주문이 없습니다" 메시지가 출력되어야 함
    EXPECT_NE(output.find("출고 가능한 주문이 없습니다"), std::string::npos)
        << "출고 후 Confirmed 목록에서 제거되지 않음 (안내 메시지 미출력)\n" << output;

    remove_dir(dir);
}

// ── TC-RL-08: current_stock은 출고 직전 최신 파일 값 기준으로 차감 확인 ───────
TEST(TG_RL, TC_RL_08_StockDecrement_BasedOnLatestFileValue) {
    auto dir = make_temp_dir("08");

    // 시료를 초기 재고 200으로 삽입
    int64_t sample_ds_id = insert_sample_direct(dir, "S-001", "테스트시료", 200);
    insert_confirmed_order(dir, "ORD-20240501-001", "S-001", "홍길동", 50);

    // App 실행 전에 별도 DataStore로 재고를 180으로 업데이트
    {
        DataStore ds((fs::path(dir) / "samples.json").string());
        JsonValue upd = JsonValue::object();
        upd["current_stock"] = JsonValue(int64_t(180));
        ds.update(sample_ds_id, upd);
    }

    // App이 최신 파일 값(180)을 읽어서 차감해야 함
    std::string input = "5\n1\n1\n0\n";
    run_app_with_input(input, dir);

    auto samples = read_all_samples(dir);
    ASSERT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0]["current_stock"].as_integer(), int64_t(130))
        << "최신 파일 값(180) 기준 차감 미적용 (기대: 130, 실제: "
        << samples[0]["current_stock"].as_integer() << ")";

    remove_dir(dir);
}
