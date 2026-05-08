#include <gtest/gtest.h>
#include "mvc/App.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string run_app_with_input(const std::string& input, const std::string& data_dir) {
    // Redirect cin
    std::istringstream in_stream(input);
    std::streambuf* old_cin = std::cin.rdbuf(in_stream.rdbuf());

    // Redirect cout
    std::ostringstream out_stream;
    std::streambuf* old_cout = std::cout.rdbuf(out_stream.rdbuf());

    mvc::AppConfig config;
    config.data_dir = data_dir;
    mvc::App app(config);
    app.run();

    // Restore
    std::cin.rdbuf(old_cin);
    std::cout.rdbuf(old_cout);

    return out_stream.str();
}

static std::string make_temp_dir(const std::string& suffix) {
    auto tmp = fs::temp_directory_path() / ("mvc_test_" + suffix);
    fs::create_directories(tmp);
    return tmp.string();
}

static void remove_dir(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

// ── TC-MVC-01: 메인 메뉴에 6개 항목 + 종료(0) 표시 ──────────────────────────
TEST(TG_MVC, TC_MVC_01_MainMenuItems) {
    auto dir = make_temp_dir("01");
    std::string output = run_app_with_input("0\n", dir);

    EXPECT_NE(output.find("1."), std::string::npos) << output;
    EXPECT_NE(output.find("2."), std::string::npos) << output;
    EXPECT_NE(output.find("3."), std::string::npos) << output;
    EXPECT_NE(output.find("4."), std::string::npos) << output;
    EXPECT_NE(output.find("5."), std::string::npos) << output;
    EXPECT_NE(output.find("6."), std::string::npos) << output;
    EXPECT_NE(output.find("0."), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-02: 메뉴 1 진입 시 시료 관리 서브 메뉴 출력 (Phase 2 이후) ───────
TEST(TG_MVC, TC_MVC_02_Menu1SampleMgmtSubMenu) {
    auto dir = make_temp_dir("02");
    // Enter menu 1, then press 0 to go back, then 0 to exit
    std::string output = run_app_with_input("1\n0\n0\n", dir);

    EXPECT_NE(output.find("시료 관리"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-03: 메뉴 2 진입 시 시료 ID 프롬프트 출력 (Phase 3 구현 완료) ──────
// Phase 3 구현 후 메뉴 2는 order_reception()을 호출하므로 "시료 ID:" 프롬프트가 출력된다.
TEST(TG_MVC, TC_MVC_03_Menu2OrderReceptionPrompt) {
    auto dir = make_temp_dir("03");
    // Enter menu 2; read_nonempty for sample ID gets EOF → returns immediately
    std::string output = run_app_with_input("2\n", dir);

    EXPECT_NE(output.find("시료 ID"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-04: 메뉴 3 진입 시 주문 처리 화면 출력 (Phase 4 구현 완료) ────────
// Phase 4 구현 후 메뉴 3은 주문 처리 화면을 표시한다.
// 주문이 없으면 "처리 대기 중인 주문이 없습니다" 메시지가 출력된다.
TEST(TG_MVC, TC_MVC_04_Menu3OrderProcessing) {
    auto dir = make_temp_dir("04");
    std::string output = run_app_with_input("3\n0\n", dir);

    EXPECT_NE(output.find("처리 대기 중인 주문이 없습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-05: 메뉴 4 진입 시 "준비 중" 출력 ────────────────────────────────
TEST(TG_MVC, TC_MVC_05_Menu4StubOutput) {
    auto dir = make_temp_dir("05");
    std::string output = run_app_with_input("4\n0\n", dir);

    EXPECT_NE(output.find("준비 중"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-06: 메뉴 5 진입 시 "준비 중" 출력 ────────────────────────────────
TEST(TG_MVC, TC_MVC_06_Menu5StubOutput) {
    auto dir = make_temp_dir("06");
    std::string output = run_app_with_input("5\n0\n", dir);

    EXPECT_NE(output.find("준비 중"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-07: 메뉴 6 진입 시 "준비 중" 출력 ────────────────────────────────
TEST(TG_MVC, TC_MVC_07_Menu6StubOutput) {
    auto dir = make_temp_dir("07");
    std::string output = run_app_with_input("6\n0\n", dir);

    EXPECT_NE(output.find("준비 중"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-08: 입력 "0"으로 프로그램 정상 종료 ──────────────────────────────
TEST(TG_MVC, TC_MVC_08_ExitOnZero) {
    auto dir = make_temp_dir("08");

    // run() should return normally (no exception, no infinite loop)
    EXPECT_NO_THROW({
        run_app_with_input("0\n", dir);
    });

    remove_dir(dir);
}

// ── TC-MVC-09: 잘못된 입력에 오류 메시지 출력 후 계속 ────────────────────────
TEST(TG_MVC, TC_MVC_09_InvalidInputError) {
    auto dir = make_temp_dir("09");
    std::string output = run_app_with_input("9\n0\n", dir);

    EXPECT_NE(output.find("잘못된 입력"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-MVC-10: 여러 메뉴를 순서대로 진입 후 종료 ────────────────────────────
// Phase 4: 메뉴 1(시료 관리), 메뉴 3(주문 처리) 모두 실제 구현 완료.
// 메뉴 1 진입 후 돌아가기, 메뉴 3 진입 후 빈 목록 안내 확인 후 종료.
TEST(TG_MVC, TC_MVC_10_MultipleMenusRun) {
    auto dir = make_temp_dir("10");
    // menu 1 (sub 0: back) → menu 3 (empty list) → 0 exit
    std::string output = run_app_with_input("1\n0\n3\n0\n", dir);

    // menu 1 서브 메뉴 진입 확인
    EXPECT_NE(output.find("시료 관리"), std::string::npos)
        << "메뉴 1 시료 관리 출력 필요\n" << output;
    // menu 3 주문 처리 진입 확인 (빈 목록)
    EXPECT_NE(output.find("처리 대기 중인 주문이 없습니다"), std::string::npos)
        << "메뉴 3 빈 목록 안내 메시지 출력 필요\n" << output;

    remove_dir(dir);
}
