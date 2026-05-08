#include <gtest/gtest.h>
#include "mvc/App.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string run_app_with_input(const std::string& input, const std::string& data_dir) {
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
    auto tmp = fs::temp_directory_path() / ("sm_test_" + suffix);
    fs::create_directories(tmp);
    return tmp.string();
}

static void remove_dir(const std::string& path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

// ── TC-SM-01: 정상 등록 → 조회로 확인 ─────────────────────────────────────────
// 시료 1건 등록 후 시료 조회 시 해당 시료 ID와 시료명이 출력에 포함된다.
TEST(TG_SM, TC_SM_01_RegisterAndList) {
    auto dir = make_temp_dir("01");

    // 메인 메뉴 1 → 서브 메뉴 1 (등록)
    // 시료 ID: S-001, 시료명: 실리콘웨이퍼, 평균생산시간: 1.5, 수율: 0.90, 현재재고: 350
    // 등록 후 서브 메뉴 2 (조회) → 0 (돌아가기) → 0 (종료)
    std::string input =
        "1\n"           // 메인 메뉴: 시료 관리
        "1\n"           // 서브 메뉴: 시료 등록
        "S-001\n"       // 시료 ID
        "실리콘웨이퍼\n"  // 시료명
        "1.5\n"         // 평균 생산시간
        "0.90\n"        // 수율
        "350\n"         // 현재 재고
        "2\n"           // 서브 메뉴: 시료 조회
        "0\n"           // 페이지 나가기 (단일 페이지면 바로 반환)
        "0\n"           // 시료 관리: 돌아가기
        "0\n";          // 종료

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("시료가 등록되었습니다"), std::string::npos) << output;
    EXPECT_NE(output.find("S-001"), std::string::npos) << output;
    EXPECT_NE(output.find("실리콘웨이퍼"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-02: 중복 시료 ID 등록 거부 ─────────────────────────────────────────
TEST(TG_SM, TC_SM_02_DuplicateSampleIdRejected) {
    auto dir = make_temp_dir("02");

    // 첫 등록 성공: S-001 / 샘플A / 1.5 / 0.90 / 100
    // 두 번째 시도: 동일 ID "S-001" → 오류 메시지 후 새 ID "S-002"로 재입력 성공
    std::string input =
        "1\n"
        "1\n"
        "S-001\n"       // 첫 번째 등록
        "샘플A\n"
        "1.5\n"
        "0.90\n"
        "100\n"
        "1\n"           // 두 번째 등록 시도
        "S-001\n"       // 중복 ID → 오류
        "S-002\n"       // 새 ID
        "샘플B\n"
        "2.0\n"
        "0.85\n"
        "50\n"
        "0\n"           // 돌아가기
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("이미 존재하는 시료 ID"), std::string::npos) << output;
    // 두 번째 등록도 완료되어야 함
    EXPECT_NE(output.find("S-002"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-03: avg_production_time 비정수·음수 입력 → 오류 후 재입력 ─────────
TEST(TG_SM, TC_SM_03_InvalidAvgProductionTime) {
    auto dir = make_temp_dir("03");

    std::string input =
        "1\n"
        "1\n"
        "S-003\n"
        "샘플C\n"
        "abc\n"         // 파싱 실패 → 오류
        "-1.0\n"        // ≤ 0 → 오류
        "0.0\n"         // = 0 → 오류
        "3.0\n"         // 유효
        "0.80\n"
        "200\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    // 오류 메시지가 여러 번 출력되어야 함
    EXPECT_NE(output.find("유효하지 않은 값"), std::string::npos) << output;
    EXPECT_NE(output.find("0보다 커야"), std::string::npos) << output;
    // 최종 등록 성공
    EXPECT_NE(output.find("시료가 등록되었습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-04: yield_rate 범위 초과 → 오류 후 재입력 ─────────────────────────
TEST(TG_SM, TC_SM_04_InvalidYieldRate) {
    auto dir = make_temp_dir("04");

    std::string input =
        "1\n"
        "1\n"
        "S-004\n"
        "샘플D\n"
        "4.0\n"
        "0.50\n"        // < 0.70 → 오류
        "1.10\n"        // > 0.99 → 오류
        "0.75\n"        // 유효
        "100\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("0.70 이상 0.99 이하"), std::string::npos) << output;
    EXPECT_NE(output.find("시료가 등록되었습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-05: current_stock 음수 → 오류 후 재입력 ───────────────────────────
TEST(TG_SM, TC_SM_05_InvalidCurrentStock) {
    auto dir = make_temp_dir("05");

    std::string input =
        "1\n"
        "1\n"
        "S-005\n"
        "샘플E\n"
        "5.0\n"
        "0.85\n"
        "-1\n"          // 음수 → read_int 오류
        "0\n"           // 유효 (0 이상)
        "0\n"           // 돌아가기
        "0\n";

    std::string output = run_app_with_input(input, dir);

    // read_int는 범위 초과 시 오류 메시지를 출력
    EXPECT_NE(output.find("잘못된 입력"), std::string::npos) << output;
    EXPECT_NE(output.find("시료가 등록되었습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-06: 시료 조회 — 빈 DB에서 안내 메시지 ──────────────────────────────
TEST(TG_SM, TC_SM_06_ListEmptyDB) {
    auto dir = make_temp_dir("06");

    std::string input =
        "1\n"
        "2\n"           // 시료 조회
        "0\n"           // 돌아가기
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("등록된 시료가 없습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-07: 시료 조회 — 여러 시료 테이블 출력 확인 ─────────────────────────
TEST(TG_SM, TC_SM_07_ListMultipleSamples) {
    auto dir = make_temp_dir("07");

    // 2건 등록 후 조회
    std::string input =
        "1\n"
        "1\n" "S-007A\n" "웨이퍼A\n" "1.0\n" "0.90\n" "100\n"
        "1\n" "S-007B\n" "웨이퍼B\n" "2.0\n" "0.85\n" "200\n"
        "2\n"           // 시료 조회
        "0\n"           // 페이지 나가기
        "0\n"           // 돌아가기
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("S-007A"), std::string::npos) << output;
    EXPECT_NE(output.find("S-007B"), std::string::npos) << output;
    // 테이블 헤더 확인
    EXPECT_NE(output.find("시료 ID"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-08: 시료명 부분 검색 — 0건 ─────────────────────────────────────────
TEST(TG_SM, TC_SM_08_SearchByNameNoResult) {
    auto dir = make_temp_dir("08");

    // 1건 등록 후 매칭 안 되는 키워드로 검색
    std::string input =
        "1\n"
        "1\n" "S-008\n" "실리콘\n" "1.0\n" "0.90\n" "100\n"
        "3\n"           // 시료 검색
        "1\n"           // 시료명 부분 검색
        "갈륨\n"         // 없는 키워드
        "0\n"           // 돌아가기
        "0\n"           // 시료 관리 돌아가기
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("검색 결과가 없습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-09: 시료명 부분 검색 — 1건 → 상세 출력 ─────────────────────────────
TEST(TG_SM, TC_SM_09_SearchByNameOneResult) {
    auto dir = make_temp_dir("09");

    std::string input =
        "1\n"
        "1\n" "S-009\n" "산화철나노\n" "2.5\n" "0.87\n" "200\n"
        "3\n"
        "1\n"
        "산화\n"         // 부분 일치 1건
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    // 상세 출력 형식 확인
    EXPECT_NE(output.find("[ 시료 상세 ]"), std::string::npos) << output;
    EXPECT_NE(output.find("S-009"), std::string::npos) << output;
    EXPECT_NE(output.find("산화철나노"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-10: 시료명 부분 검색 — 복수건 → 테이블 출력 ────────────────────────
TEST(TG_SM, TC_SM_10_SearchByNameMultipleResults) {
    auto dir = make_temp_dir("10");

    // "나노"가 이름에 포함된 시료 2건 등록
    std::string input =
        "1\n"
        "1\n" "S-010A\n" "산화철나노A\n" "1.0\n" "0.90\n" "100\n"
        "1\n" "S-010B\n" "탄소나노B\n"   "2.0\n" "0.85\n" "200\n"
        "3\n"
        "1\n"
        "나노\n"
        "0\n"           // 페이지 나가기 (테이블)
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    // 상세가 아닌 테이블 출력이어야 함 (헤더 포함)
    EXPECT_NE(output.find("시료 ID"), std::string::npos) << output;
    EXPECT_NE(output.find("S-010A"), std::string::npos) << output;
    EXPECT_NE(output.find("S-010B"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-11: 시료 ID 검색 — 존재 → 상세 출력 ────────────────────────────────
TEST(TG_SM, TC_SM_11_SearchBySampleIdFound) {
    auto dir = make_temp_dir("11");

    std::string input =
        "1\n"
        "1\n" "S-011\n" "산화아연\n" "3.0\n" "0.88\n" "150\n"
        "3\n"
        "2\n"           // 시료 ID 검색
        "S-011\n"
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("[ 시료 상세 ]"), std::string::npos) << output;
    EXPECT_NE(output.find("S-011"), std::string::npos) << output;
    EXPECT_NE(output.find("산화아연"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-12: 시료 ID 검색 — 미존재 → 안내 메시지 ────────────────────────────
TEST(TG_SM, TC_SM_12_SearchBySampleIdNotFound) {
    auto dir = make_temp_dir("12");

    std::string input =
        "1\n"
        "1\n" "S-012\n" "갈륨비소\n" "4.0\n" "0.82\n" "80\n"
        "3\n"
        "2\n"
        "S-999\n"       // 없는 ID
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("검색 결과가 없습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-13: 등록 완료 후 시료 ID 출력 확인 (FR-S-03) ───────────────────────
TEST(TG_SM, TC_SM_13_RegisterOutputsSampleId) {
    auto dir = make_temp_dir("13");

    std::string input =
        "1\n"
        "1\n"
        "S-013\n"
        "인화인듐\n"
        "6.0\n"
        "0.78\n"
        "50\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    // FR-S-03: 등록 완료 후 부여된 시료 ID 출력
    EXPECT_NE(output.find("시료가 등록되었습니다"), std::string::npos) << output;
    EXPECT_NE(output.find("S-013"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-14: 중복 시료명 등록 거부 ──────────────────────────────────────────
TEST(TG_SM, TC_SM_14_DuplicateSampleNameRejected) {
    auto dir = make_temp_dir("14");

    std::string input =
        "1\n"
        "1\n" "S-014A\n" "이름중복\n"   "1.0\n" "0.90\n" "100\n"
        "1\n"
        "S-014B\n"
        "이름중복\n"   // 중복 시료명 → 오류
        "새이름\n"     // 새 이름으로 재입력
        "2.0\n"
        "0.85\n"
        "50\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("이미 존재하는 시료명"), std::string::npos) << output;
    // 두 번째 등록 완료: S-014B ID가 출력되어야 함
    EXPECT_NE(output.find("S-014B"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-15: 중복 avg_production_time 등록 거부 ─────────────────────────────
TEST(TG_SM, TC_SM_15_DuplicateAvgProductionTimeRejected) {
    auto dir = make_temp_dir("15");

    std::string input =
        "1\n"
        "1\n" "S-015A\n" "샘플P\n" "7.5\n" "0.90\n" "100\n"
        "1\n"
        "S-015B\n"
        "샘플Q\n"
        "7.5\n"         // 중복 평균 생산시간 → 오류
        "8.0\n"         // 새 값
        "0.85\n"
        "50\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("이미 존재하는 평균 생산시간"), std::string::npos) << output;
    EXPECT_NE(output.find("시료가 등록되었습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-16: 중복 yield_rate 등록 거부 ──────────────────────────────────────
TEST(TG_SM, TC_SM_16_DuplicateYieldRateRejected) {
    auto dir = make_temp_dir("16");

    std::string input =
        "1\n"
        "1\n" "S-016A\n" "샘플R\n" "1.0\n" "0.92\n" "100\n"
        "1\n"
        "S-016B\n"
        "샘플S\n"
        "9.0\n"
        "0.92\n"        // 중복 수율 → 오류
        "0.93\n"        // 새 값
        "50\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("이미 존재하는 수율"), std::string::npos) << output;
    EXPECT_NE(output.find("시료가 등록되었습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-17: 수율 정확 검색 — 존재 → 상세 출력 ─────────────────────────────
TEST(TG_SM, TC_SM_17_SearchByYieldRateFound) {
    auto dir = make_temp_dir("17");

    std::string input =
        "1\n"
        "1\n" "S-017\n" "탄탈룸\n" "10.0\n" "0.77\n" "30\n"
        "3\n"
        "3\n"           // 수율 정확 검색
        "0.77\n"
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("[ 시료 상세 ]"), std::string::npos) << output;
    EXPECT_NE(output.find("S-017"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-18: 수율 정확 검색 — 미존재 → 안내 메시지 ─────────────────────────
TEST(TG_SM, TC_SM_18_SearchByYieldRateNotFound) {
    auto dir = make_temp_dir("18");

    std::string input =
        "1\n"
        "1\n" "S-018\n" "텅스텐\n" "11.0\n" "0.76\n" "20\n"
        "3\n"
        "3\n"
        "0.99\n"        // 존재하지 않는 수율
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("검색 결과가 없습니다"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-19: 평균 생산시간 정확 검색 — 존재 → 상세 출력 ────────────────────
TEST(TG_SM, TC_SM_19_SearchByAvgProductionTimeFound) {
    auto dir = make_temp_dir("19");

    std::string input =
        "1\n"
        "1\n" "S-019\n" "티타늄\n" "12.5\n" "0.73\n" "40\n"
        "3\n"
        "4\n"           // 평균 생산시간 정확 검색
        "12.5\n"
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("[ 시료 상세 ]"), std::string::npos) << output;
    EXPECT_NE(output.find("S-019"), std::string::npos) << output;

    remove_dir(dir);
}

// ── TC-SM-20: 평균 생산시간 정확 검색 — 미존재 → 안내 메시지 ────────────────
TEST(TG_SM, TC_SM_20_SearchByAvgProductionTimeNotFound) {
    auto dir = make_temp_dir("20");

    std::string input =
        "1\n"
        "1\n" "S-020\n" "몰리브덴\n" "13.0\n" "0.74\n" "10\n"
        "3\n"
        "4\n"
        "99.9\n"        // 존재하지 않는 평균 생산시간
        "0\n"
        "0\n"
        "0\n";

    std::string output = run_app_with_input(input, dir);

    EXPECT_NE(output.find("검색 결과가 없습니다"), std::string::npos) << output;

    remove_dir(dir);
}
