#pragma once
#include <cstdint>
#include <string>

namespace ProductionCalc {
    // Actual production quantity: ceil(shortage / (yield_rate * 0.9))
    // Returns 0 if shortage == 0.
    int64_t actual_production(int64_t shortage, double yield_rate);

    // Total production time in minutes:
    //   actual_production * avg_production_time (hours) * 60
    double estimated_minutes(int64_t actual_production_qty, double avg_production_time);

    // Convert total minutes to "HH:MM" string (rounded, always 2 digits each).
    // e.g. 210.4 -> "03:30"
    std::string format_duration(double total_minutes);
}
