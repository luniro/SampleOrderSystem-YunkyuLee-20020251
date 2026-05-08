// tests/mvc/test_order_reception.cpp — TG-OR: Phase 3 주문 접수 테스트
#include <gtest/gtest.h>
#include "mvc/App.hpp"
#include "data_store.hpp"

#include <filesystem>
#include <iostream>
#include <regex>
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
    auto tmp = fs::temp_directory_path() / ("or_test_" + suffix);
    // Clean up any leftover state from previous test runs
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp);
    return tmp.string();
}

static void remove_dir(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

// ── TC-OR-01: 정상 접수 → 주문번호 형식 확인 ──────────────────────────────────
// 접수 완료 후 출력에 ORD-YYYYMMDD-NNN 형식 주문번호가 포함된다.
TEST(TG_OR, TC_OR_01_NormalReception_OrderNumberFormat) {
    auto dir = make_temp_dir("01");

    // 시료 등록 후 주문 접수 (동일 App 세션)
    std::string input =
        "1\n"           // 메인 메뉴: 시료 관리
        "1\n"           // 서브 메뉴: 시료 등록
        "S-001\n"       // 시료 ID
        "실리콘웨이퍼\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"           // 서브 메뉴: 돌아가기
        "2\n"           // 메인 메뉴: 주문 접수 (order reception)
        "S-001\n"       // 시료 ID
        "홍길동\n"       // 고객명
        "100\n"         // 주문수량
        "1\n"           // 접수 확인
        "0\n";          // 종료

    std::string output = run_app_with_input(input, dir);

    // ORD-YYYYMMDD-NNN 형식 확인 (정규식)
    std::regex re("ORD-[0-9]{8}-[0-9]{3,}");
    EXPECT_TRUE(std::regex_search(output, re))
        << "주문번호 형식 ORD-YYYYMMDD-NNN 미발견\n" << output;
    EXPECT_NE(output.find("주문이 접수되었습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-OR-02: 접수 완료 후 orders.json에 Reserved 상태로 저장 확인 ──────────────
TEST(TG_OR, TC_OR_02_SavedAsReserved) {
    auto dir = make_temp_dir("02");

    std::string input =
        "1\n"
        "1\n"
        "S-001\n"
        "실리콘웨이퍼\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"
        "2\n"
        "S-001\n"
        "김철수\n"
        "50\n"
        "1\n"
        "0\n";

    run_app_with_input(input, dir);

    // orders.json에 Reserved(0) 상태로 저장되었는지 확인
    std::string orders_path = (fs::path(dir) / "orders.json").string();
    ASSERT_TRUE(fs::exists(orders_path)) << "orders.json 파일이 존재하지 않음";

    DataStore ds(orders_path);
    auto records = ds.read_all();
    ASSERT_EQ(static_cast<int>(records.size()), 1) << "레코드가 1건이어야 함";

    auto& rec = records[0];
    EXPECT_EQ(rec["order_status"].as_integer(), 0)   // STATUS_RESERVED == 0
        << "order_status가 Reserved(0)이어야 함";
    EXPECT_EQ(rec["customer_name"].as_string(), "김철수");
    EXPECT_EQ(rec["order_quantity"].as_integer(), 50LL);
    EXPECT_EQ(rec["sample_id"].as_string(), "S-001");

    remove_dir(dir);
}

// ── TC-OR-03: 존재하지 않는 시료 ID → 오류 메시지 후 재입력 (FR-O-02) ──────────
TEST(TG_OR, TC_OR_03_InvalidSampleId_ErrorAndRetry) {
    auto dir = make_temp_dir("03");

    std::string input =
        "1\n"
        "1\n"
        "S-001\n"
        "실리콘웨이퍼\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"
        "2\n"
        "INVALID-ID\n"  // 존재하지 않는 시료 ID
        "S-001\n"       // 올바른 시료 ID로 재입력
        "이영희\n"
        "30\n"
        "1\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("존재하지 않는 시료 ID"), std::string::npos)
        << "오류 메시지 미출력\n" << output;
    EXPECT_NE(output.find("주문이 접수되었습니다"), std::string::npos)
        << "재입력 후 접수 완료 메시지 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-OR-04: 취소 선택 → 주문 미저장 확인 (FR-O-04) ────────────────────────────
TEST(TG_OR, TC_OR_04_Cancel_NoOrderSaved) {
    auto dir = make_temp_dir("04");

    std::string input =
        "1\n"
        "1\n"
        "S-001\n"
        "실리콘웨이퍼\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"
        "2\n"
        "S-001\n"
        "박민준\n"
        "20\n"
        "0\n"           // 취소 선택
        "0\n";

    run_app_with_input(input, dir);

    // orders.json이 없거나 레코드가 0건이어야 한다
    std::string orders_path = (fs::path(dir) / "orders.json").string();
    if (fs::exists(orders_path)) {
        DataStore ds(orders_path);
        EXPECT_EQ(static_cast<int>(ds.read_all().size()), 0)
            << "취소 선택 후 주문이 저장되면 안 됨";
    }
    // 파일이 없으면 저장 안 된 것이므로 패스

    remove_dir(dir);
}

// ── TC-OR-05: 주문수량 0 또는 음수 입력 → 오류 후 재입력 ──────────────────────────
TEST(TG_OR, TC_OR_05_InvalidQuantity_ErrorAndRetry) {
    auto dir = make_temp_dir("05");

    std::string input =
        "1\n"
        "1\n"
        "S-001\n"
        "실리콘웨이퍼\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"
        "2\n"
        "S-001\n"
        "최지수\n"
        "0\n"           // 잘못된 수량: 0
        "-1\n"          // 잘못된 수량: -1
        "10\n"          // 올바른 수량
        "1\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("잘못된 입력"), std::string::npos)
        << "수량 오류 메시지 미출력\n" << output;
    EXPECT_NE(output.find("주문이 접수되었습니다"), std::string::npos)
        << "재입력 후 접수 완료 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-OR-06: 동일 날짜 두 번째 접수 → NNN = 002 ────────────────────────────────
TEST(TG_OR, TC_OR_06_SecondOrderSameDay_SeqIs002) {
    auto dir = make_temp_dir("06");

    // 첫 번째 주문 접수 (시료 등록 포함)
    std::string input_first =
        "1\n"
        "1\n"
        "S-001\n"
        "실리콘웨이퍼\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"
        "2\n"
        "S-001\n"
        "첫번째고객\n"
        "10\n"
        "1\n"
        "0\n";

    run_app_with_input(input_first, dir);

    // 두 번째 주문 접수 (새 App 인스턴스, 동일 data_dir)
    std::string input_second =
        "2\n"
        "S-001\n"
        "두번째고객\n"
        "20\n"
        "1\n"
        "0\n";

    std::string output = run_app_with_input(input_second, dir);

    // 두 번째 주문번호에 -002 가 포함되어야 한다
    EXPECT_NE(output.find("-002"), std::string::npos)
        << "두 번째 주문번호 시퀀스가 002이어야 함\n" << output;

    remove_dir(dir);
}

// ── TC-OR-07: 확인 화면에 시료명·시료ID·고객명·수량 표시 확인 (FR-O-03) ───────────
TEST(TG_OR, TC_OR_07_ConfirmScreenContent) {
    auto dir = make_temp_dir("07");

    std::string input =
        "1\n"
        "1\n"
        "S-007\n"
        "나노입자\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"
        "2\n"
        "S-007\n"
        "홍길동\n"
        "77\n"
        "0\n"           // 취소 (확인 화면만 확인)
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("[ 주문 확인 ]"), std::string::npos)
        << "확인 화면 헤더 미출력\n" << output;
    EXPECT_NE(output.find("나노입자"), std::string::npos)
        << "시료명 미출력\n" << output;
    EXPECT_NE(output.find("S-007"), std::string::npos)
        << "시료 ID 미출력\n" << output;
    EXPECT_NE(output.find("홍길동"), std::string::npos)
        << "고객명 미출력\n" << output;
    EXPECT_NE(output.find("77 ea"), std::string::npos)
        << "수량 미출력\n" << output;

    remove_dir(dir);
}

// ── TC-OR-08: approved_at, released_at 이 null로 저장 확인 ──────────────────────
TEST(TG_OR, TC_OR_08_NullTimestampsOnReception) {
    auto dir = make_temp_dir("08");

    std::string input =
        "1\n"
        "1\n"
        "S-001\n"
        "실리콘웨이퍼\n"
        "1.5\n"
        "0.90\n"
        "350\n"
        "0\n"
        "2\n"
        "S-001\n"
        "테스트고객\n"
        "5\n"
        "1\n"
        "0\n";

    run_app_with_input(input, dir);

    std::string orders_path = (fs::path(dir) / "orders.json").string();
    ASSERT_TRUE(fs::exists(orders_path));

    DataStore ds(orders_path);
    auto records = ds.read_all();
    ASSERT_EQ(static_cast<int>(records.size()), 1);

    auto& rec = records[0];
    EXPECT_TRUE(rec["approved_at"].is_null())
        << "approved_at이 null이어야 함";
    EXPECT_TRUE(rec["released_at"].is_null())
        << "released_at이 null이어야 함";

    remove_dir(dir);
}
