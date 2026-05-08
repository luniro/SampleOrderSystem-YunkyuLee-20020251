#include <gtest/gtest.h>
#include "util/timestamp.hpp"
#include "util/production_calc.hpp"

// ============================================================
// TG-TS: Timestamp utilities
// ============================================================

// TC-TS-01: parse() — valid timestamp (KST, round-trip)
TEST(TG_TS, ParseValid) {
    int64_t ep = Timestamp::parse("2024-05-01 10:30:00");
    // 2024-05-01 10:30:00 KST -> round-trip via format() -> same KST string + suffix
    std::string back = Timestamp::format(ep);
    EXPECT_EQ(back, "2024-05-01 10:30:00 (KST)");
}

// TC-TS-02: parse() — epoch zero / invalid string
TEST(TG_TS, ParseInvalid) {
    EXPECT_EQ(Timestamp::parse(""), 0);
    EXPECT_EQ(Timestamp::parse("not-a-date"), 0);
}

// TC-TS-03: format() — epoch 0 -> 1970-01-01 09:00:00 (KST)
TEST(TG_TS, FormatEpochZero) {
    EXPECT_EQ(Timestamp::format(0), "1970-01-01 09:00:00 (KST)");
}

// TC-TS-04: parse/format round-trip (KST)
TEST(TG_TS, ParseFormatRoundTrip) {
    const std::string ts = "2024-12-31 23:59:59";
    EXPECT_EQ(Timestamp::format(Timestamp::parse(ts)), ts + " (KST)");
}

// TC-TS-05: now() — "YYYY-MM-DD HH:MM:SS (KST)" format (25 chars)
TEST(TG_TS, NowFormat) {
    std::string n = Timestamp::now();
    EXPECT_EQ(n.size(), 25u);
    EXPECT_EQ(n[4],  '-');
    EXPECT_EQ(n[7],  '-');
    EXPECT_EQ(n[10], ' ');
    EXPECT_EQ(n[13], ':');
    EXPECT_EQ(n[16], ':');
    EXPECT_EQ(n.substr(19), " (KST)");
}

// TC-TS-06: parse_duration_minutes() — "03:30" -> 210
TEST(TG_TS, ParseDurationMinutes_Normal) {
    EXPECT_EQ(Timestamp::parse_duration_minutes("03:30"), 210);
}

// TC-TS-07: parse_duration_minutes() — "00:00" -> 0
TEST(TG_TS, ParseDurationMinutes_Zero) {
    EXPECT_EQ(Timestamp::parse_duration_minutes("00:00"), 0);
}

// TC-TS-08: parse_duration_minutes() — "06:00" -> 360
TEST(TG_TS, ParseDurationMinutes_SixHours) {
    EXPECT_EQ(Timestamp::parse_duration_minutes("06:00"), 360);
}

// TC-TS-09: parse_duration_minutes() — invalid -> 0
TEST(TG_TS, ParseDurationMinutes_Invalid) {
    EXPECT_EQ(Timestamp::parse_duration_minutes(""), 0);
    EXPECT_EQ(Timestamp::parse_duration_minutes("abc"), 0);
}

// TC-TS-10: completion_epoch() — start + duration (KST)
TEST(TG_TS, CompletionEpoch_Normal) {
    // "2024-05-01 10:30:00 KST" + "03:30" = "2024-05-01 14:00:00 KST"
    int64_t ep = Timestamp::completion_epoch("2024-05-01 10:30:00", "03:30");
    EXPECT_EQ(Timestamp::format(ep), "2024-05-01 14:00:00 (KST)");
}

// TC-TS-11: completion_epoch() — across midnight (KST)
TEST(TG_TS, CompletionEpoch_AcrossMidnight) {
    // "2024-05-01 22:00:00 KST" + "03:00" = "2024-05-02 01:00:00 KST"
    int64_t ep = Timestamp::completion_epoch("2024-05-01 22:00:00", "03:00");
    EXPECT_EQ(Timestamp::format(ep), "2024-05-02 01:00:00 (KST)");
}

// TC-TS-12: format_completion() — same day -> "HH:MM"
TEST(TG_TS, FormatCompletion_SameDay) {
    // Both in 2024-05-01 UTC (same day-index)
    int64_t comp = Timestamp::parse("2024-05-01 14:00:00");
    int64_t now  = Timestamp::parse("2024-05-01 10:00:00");
    std::string result = Timestamp::format_completion(comp, now);
    EXPECT_EQ(result, "14:00");
}

// TC-TS-13: format_completion() — next day -> "HH:MM (+1 day)"
TEST(TG_TS, FormatCompletion_NextDay) {
    int64_t comp = Timestamp::parse("2024-05-02 02:00:00");
    int64_t now  = Timestamp::parse("2024-05-01 23:00:00");
    std::string result = Timestamp::format_completion(comp, now);
    EXPECT_EQ(result, "02:00 (+1 day)");
}

// TC-TS-14: format_completion() — two days later -> "HH:MM (+2 days)"
TEST(TG_TS, FormatCompletion_TwoDays) {
    int64_t comp = Timestamp::parse("2024-05-03 06:00:00");
    int64_t now  = Timestamp::parse("2024-05-01 10:00:00");
    std::string result = Timestamp::format_completion(comp, now);
    EXPECT_EQ(result, "06:00 (+2 days)");
}

// TC-TS-15: format_completion() — N <= 0 (already past) -> "HH:MM"
TEST(TG_TS, FormatCompletion_AlreadyPast) {
    int64_t comp = Timestamp::parse("2024-05-01 08:00:00");
    int64_t now  = Timestamp::parse("2024-05-01 10:00:00");
    std::string result = Timestamp::format_completion(comp, now);
    EXPECT_EQ(result, "08:00");
}

// TC-TS-16: calc_progress() — in-progress (50%)
TEST(TG_TS, CalcProgress_InProgress) {
    // start=0, duration=60min, now=30min -> 50%
    int64_t start_ep = Timestamp::parse("2024-05-01 00:00:00");
    int64_t now_ep   = start_ep + 30 * 60; // 30 minutes later
    double p = Timestamp::calc_progress("2024-05-01 00:00:00", "01:00", now_ep);
    EXPECT_NEAR(p, 50.0, 0.001);
}

// TC-TS-17: calc_progress() — not started yet (0%)
TEST(TG_TS, CalcProgress_NotStarted) {
    int64_t start_ep = Timestamp::parse("2024-05-01 12:00:00");
    int64_t now_ep   = start_ep - 60; // 1 min before start
    double p = Timestamp::calc_progress("2024-05-01 12:00:00", "01:00", now_ep);
    EXPECT_NEAR(p, 0.0, 0.001);
}

// TC-TS-18: calc_progress() — completed (100%)
TEST(TG_TS, CalcProgress_Completed) {
    int64_t start_ep = Timestamp::parse("2024-05-01 00:00:00");
    int64_t now_ep   = start_ep + 120 * 60; // 2h after start, duration=1h
    double p = Timestamp::calc_progress("2024-05-01 00:00:00", "01:00", now_ep);
    EXPECT_NEAR(p, 100.0, 0.001);
}

// TC-TS-19: calc_progress() — zero duration -> 100.0
TEST(TG_TS, CalcProgress_ZeroDuration) {
    int64_t now_ep = Timestamp::parse("2024-05-01 10:00:00");
    double p = Timestamp::calc_progress("2024-05-01 10:00:00", "00:00", now_ep);
    EXPECT_NEAR(p, 100.0, 0.001);
}

// ============================================================
// TG-PC: ProductionCalc utilities
// ============================================================

// TC-PC-01: actual_production() — shortage=0 -> 0
TEST(TG_PC, ActualProduction_ZeroShortage) {
    EXPECT_EQ(ProductionCalc::actual_production(0, 0.87), 0);
}

// TC-PC-02: actual_production() — typical case
TEST(TG_PC, ActualProduction_Typical) {
    // ceil(20 / (0.87 * 0.9)) = ceil(20 / 0.783) = ceil(25.54...) = 26
    EXPECT_EQ(ProductionCalc::actual_production(20, 0.87), 26);
}

// TC-PC-03: actual_production() — exact division
TEST(TG_PC, ActualProduction_ExactDivision) {
    // ceil(90 / (1.0 * 0.9)) = ceil(100.0) = 100
    EXPECT_EQ(ProductionCalc::actual_production(90, 1.0), 100);
}

// TC-PC-04: actual_production() — shortage=1, low yield
TEST(TG_PC, ActualProduction_OneShortage) {
    // ceil(1 / (0.5 * 0.9)) = ceil(1/0.45) = ceil(2.222...) = 3
    EXPECT_EQ(ProductionCalc::actual_production(1, 0.5), 3);
}

// TC-PC-05: estimated_minutes() — typical
TEST(TG_PC, EstimatedMinutes_Typical) {
    // 23 * 4.5 = 103.5  (avg_production_time 단위: min/ea)
    EXPECT_NEAR(ProductionCalc::estimated_minutes(23, 4.5), 103.5, 0.001);
}

// TC-PC-06: estimated_minutes() — zero production
TEST(TG_PC, EstimatedMinutes_ZeroProduction) {
    EXPECT_NEAR(ProductionCalc::estimated_minutes(0, 4.5), 0.0, 0.001);
}

// TC-PC-07: format_duration() — 210 -> "03:30"
TEST(TG_PC, FormatDuration_210) {
    EXPECT_EQ(ProductionCalc::format_duration(210.0), "03:30");
}

// TC-PC-08: format_duration() — 210.4 -> "03:30" (round down)
TEST(TG_PC, FormatDuration_210_4) {
    EXPECT_EQ(ProductionCalc::format_duration(210.4), "03:30");
}

// TC-PC-09: format_duration() — 210.5 -> "03:31" (round half-up)
TEST(TG_PC, FormatDuration_210_5) {
    EXPECT_EQ(ProductionCalc::format_duration(210.5), "03:31");
}

// TC-PC-10: format_duration() — 0 -> "00:00"
TEST(TG_PC, FormatDuration_Zero) {
    EXPECT_EQ(ProductionCalc::format_duration(0.0), "00:00");
}

// TC-PC-11: format_duration() — 60 -> "01:00"
TEST(TG_PC, FormatDuration_60) {
    EXPECT_EQ(ProductionCalc::format_duration(60.0), "01:00");
}

// TC-PC-12: format_duration() — 360 -> "06:00"
TEST(TG_PC, FormatDuration_360) {
    EXPECT_EQ(ProductionCalc::format_duration(360.0), "06:00");
}

// TC-PC-13: format_duration() — large value (>= 100h)
TEST(TG_PC, FormatDuration_Large) {
    // 6210.0 -> 103:30
    EXPECT_EQ(ProductionCalc::format_duration(6210.0), "103:30");
}
