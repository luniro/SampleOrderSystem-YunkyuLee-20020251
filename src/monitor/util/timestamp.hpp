#pragma once
#include <cstdint>
#include <string>

namespace Timestamp {
    // "YYYY-MM-DD HH:MM:SS" -> UTC epoch seconds (int64_t)
    // Returns 0 on parse failure.
    int64_t parse(const std::string& ts);

    // UTC epoch seconds -> "YYYY-MM-DD HH:MM:SS"
    std::string format(int64_t epoch);

    // Current UTC time -> "YYYY-MM-DD HH:MM:SS"
    std::string now();

    // "HH:MM" duration string -> total minutes (int64_t)
    // e.g. "03:30" -> 210
    int64_t parse_duration_minutes(const std::string& hhmm);

    // production_start_at ("YYYY-MM-DD HH:MM:SS") +
    // estimated_completion ("HH:MM") -> completion epoch seconds
    int64_t completion_epoch(const std::string& production_start_at,
                             const std::string& estimated_completion);

    // Format completion time for display:
    // - Same UTC date as now: "HH:MM"
    // - Next day or later:    "HH:MM (+N day)" / "HH:MM (+N days)"
    // - N <= 0 (already past or same day): "HH:MM"
    std::string format_completion(int64_t completion_epoch_val, int64_t now_epoch);

    // Production progress [0.0, 100.0]
    // elapsed / total * 100, clamped to [0, 100].
    // Returns 100.0 if total duration is 0.
    double calc_progress(const std::string& production_start_at,
                         const std::string& estimated_completion,
                         int64_t now_epoch);
}
