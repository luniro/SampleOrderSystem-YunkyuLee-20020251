#include "util/production_calc.hpp"

#include <cmath>
#include <sstream>
#include <iomanip>

namespace ProductionCalc {

int64_t actual_production(int64_t shortage, double yield_rate) {
    if (shortage == 0) return 0;
    double effective = yield_rate * 0.9;
    double result = std::ceil(static_cast<double>(shortage) / effective);
    return static_cast<int64_t>(result);
}

double estimated_minutes(int64_t actual_production_qty, double avg_production_time) {
    return static_cast<double>(actual_production_qty) * avg_production_time * 60.0;
}

std::string format_duration(double total_minutes) {
    long long total_mins = static_cast<long long>(std::round(total_minutes));
    if (total_mins < 0) total_mins = 0;
    long long hh = total_mins / 60;
    long long mm = total_mins % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hh
        << ':'
        << std::setfill('0') << std::setw(2) << mm;
    return oss.str();
}

} // namespace ProductionCalc
