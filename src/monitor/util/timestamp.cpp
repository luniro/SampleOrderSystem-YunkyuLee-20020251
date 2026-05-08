#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "util/timestamp.hpp"

#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace Timestamp {

// ---------------------------------------------------------------------------
// parse: "YYYY-MM-DD HH:MM:SS (KST)" -> UTC epoch seconds
//        " (KST)" suffix is optional for backward compatibility.
//        The datetime portion is always interpreted as KST (UTC+9).
// ---------------------------------------------------------------------------
int64_t parse(const std::string& ts) {
    // Strip optional " (KST)" suffix
    std::string s = ts;
    auto kst_pos = s.find(" (KST)");
    if (kst_pos != std::string::npos) s.erase(kst_pos);

    if (s.size() < 19) return 0;

    int year = 0, mon = 0, day = 0, hour = 0, min = 0, sec = 0;
    int n = std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d",
                        &year, &mon, &day, &hour, &min, &sec);
    if (n != 6) return 0;

    struct tm t{};
    t.tm_year = year - 1900;
    t.tm_mon  = mon - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = min;
    t.tm_sec  = sec;
    t.tm_isdst = 0;

    // _mkgmtime / timegm treats tm as UTC; subtract KST offset to get true UTC epoch
#ifdef _WIN32
    time_t epoch = _mkgmtime(&t);
#else
    time_t epoch = timegm(&t);
#endif
    if (epoch == static_cast<time_t>(-1)) return 0;
    return static_cast<int64_t>(epoch) - 9 * 3600;
}

// ---------------------------------------------------------------------------
// format: UTC epoch seconds -> "YYYY-MM-DD HH:MM:SS (KST)"
// ---------------------------------------------------------------------------
std::string format(int64_t epoch) {
    time_t t = static_cast<time_t>(epoch + 9 * 3600);  // shift to KST
    struct tm tm_val{};

#ifdef _WIN32
    if (gmtime_s(&tm_val, &t) != 0) return "1970-01-01 09:00:00 (KST)";
#else
    if (gmtime_r(&t, &tm_val) == nullptr) return "1970-01-01 09:00:00 (KST)";
#endif

    char buf[20];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm_val.tm_year + 1900, tm_val.tm_mon + 1, tm_val.tm_mday,
                  tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
    return std::string(buf) + " (KST)";
}

// ---------------------------------------------------------------------------
// now: current UTC time -> "YYYY-MM-DD HH:MM:SS"
// ---------------------------------------------------------------------------
std::string now() {
    time_t t = std::time(nullptr);
    return format(static_cast<int64_t>(t));
}

// ---------------------------------------------------------------------------
// parse_duration_minutes: "HH:MM" -> total minutes
// ---------------------------------------------------------------------------
int64_t parse_duration_minutes(const std::string& hhmm) {
    if (hhmm.size() < 5) return 0;
    int h = 0, m = 0;
    int n = std::sscanf(hhmm.c_str(), "%d:%d", &h, &m);
    if (n != 2) return 0;
    return static_cast<int64_t>(h) * 60 + static_cast<int64_t>(m);
}

// ---------------------------------------------------------------------------
// completion_epoch: start + duration -> epoch
// ---------------------------------------------------------------------------
int64_t completion_epoch(const std::string& production_start_at,
                         const std::string& estimated_completion) {
    int64_t start = parse(production_start_at);
    int64_t dur_min = parse_duration_minutes(estimated_completion);
    return start + dur_min * 60;
}

// ---------------------------------------------------------------------------
// format_completion: epoch -> "HH:MM" or "HH:MM (+N day(s))" in KST
// ---------------------------------------------------------------------------
std::string format_completion(int64_t completion_epoch_val, int64_t now_epoch) {
    // Extract HH:MM in KST
    time_t t = static_cast<time_t>(completion_epoch_val + 9 * 3600);
    struct tm tm_c{};
#ifdef _WIN32
    if (gmtime_s(&tm_c, &t) != 0) return "00:00";
#else
    if (gmtime_r(&t, &tm_c) == nullptr) return "00:00";
#endif

    char buf[6];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", tm_c.tm_hour, tm_c.tm_min);
    std::string hhmm(buf);

    // Day difference in KST (midnight boundary at UTC+9)
    int64_t completion_day = (completion_epoch_val + 9 * 3600) / 86400;
    int64_t now_day        = (now_epoch        + 9 * 3600) / 86400;
    int64_t N = completion_day - now_day;

    if (N <= 0) {
        return hhmm;
    } else if (N == 1) {
        return hhmm + " (+1 day)";
    } else {
        return hhmm + " (+" + std::to_string(N) + " days)";
    }
}

// ---------------------------------------------------------------------------
// calc_progress: [0.0, 100.0]
// ---------------------------------------------------------------------------
double calc_progress(const std::string& production_start_at,
                     const std::string& estimated_completion,
                     int64_t now_epoch) {
    int64_t start = parse(production_start_at);
    int64_t total_min = parse_duration_minutes(estimated_completion);

    if (total_min == 0) return 100.0;

    double total_sec = static_cast<double>(total_min) * 60.0;
    double elapsed   = static_cast<double>(now_epoch - start);

    double progress = (elapsed / total_sec) * 100.0;
    if (progress < 0.0)   progress = 0.0;
    if (progress > 100.0) progress = 100.0;
    return progress;
}

} // namespace Timestamp
