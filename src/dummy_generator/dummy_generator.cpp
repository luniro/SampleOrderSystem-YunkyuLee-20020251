#include "dummy_generator.hpp"
#include "data_store.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

const std::vector<std::string> kMaterials = {
    "실리콘", "산화막", "GaN", "SiC", "InP", "GaAs", "사파이어", "게르마늄"
};

const std::vector<std::string> kWaferTypes = {
    "웨이퍼", "에피텍셜", "파워기판", "기판"
};

const std::vector<std::string> kSizes = {
    "2인치", "4인치", "6인치", "8인치", "12인치"
};

const std::vector<std::string> kCustomerNames = {
    "홍길동", "김철수", "이영희", "박민준", "최수진",
    "정태양", "강지원", "윤서연", "임도현", "오지현"
};

std::string format_epoch(int64_t epoch) {
    std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M");
    return oss.str();
}

std::string format_duration_hhmm(double minutes) {
    auto total_mins = static_cast<int64_t>(std::round(minutes));
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << (total_mins / 60) << ":"
        << std::setw(2) << std::setfill('0') << (total_mins % 60);
    return oss.str();
}

std::string fmt_sample_id(int n) {
    std::ostringstream oss;
    oss << "S-";
    if (n < 1000)
        oss << std::setw(3) << std::setfill('0') << n;
    else
        oss << n;
    return oss.str();
}

std::string fmt_order_number(int n) {
    std::ostringstream oss;
    oss << "ORD-20240501-" << std::setw(3) << std::setfill('0') << n;
    return oss.str();
}

struct SampleInfo {
    std::string sample_id;
    std::string sample_name;
    double      yield_rate;
    double      avg_production_time;
};

struct OrderInfo {
    std::string order_number;
    std::string sample_name;
    int64_t     order_quantity;
    double      yield_rate;
    double      avg_production_time;
};

} // namespace

void generate_dummy_data(const GeneratorConfig& config) {
    if (config.sample_count <= 0 || config.order_count <= 0 || config.production_count <= 0)
        throw std::invalid_argument("all counts must be positive");

    fs::create_directories(config.output_dir);

    std::mt19937 rng(42);

    // 2024-01-01T00:00:00 UTC
    const int64_t BASE_EPOCH = 1704067200LL;

    // ── Samples ──────────────────────────────────────────────────────────────
    DataStore sample_store((fs::path(config.output_dir) / "samples.json").string());
    std::vector<SampleInfo> samples;
    samples.reserve(static_cast<size_t>(config.sample_count));

    std::uniform_int_distribution<int> mat_pick(0, static_cast<int>(kMaterials.size()) - 1);
    std::uniform_int_distribution<int> type_pick(0, static_cast<int>(kWaferTypes.size()) - 1);
    std::uniform_int_distribution<int> size_pick(0, static_cast<int>(kSizes.size()) - 1);
    std::uniform_real_distribution<double> pt_low(0.1, 1.0);
    std::uniform_real_distribution<double> pt_high(1.0, 2.0);
    std::uniform_real_distribution<double> pt_selector(0.0, 1.0);
    std::normal_distribution<double>       yield_nd(0.90, 0.07);
    std::uniform_int_distribution<int>     stock_dist(50, 500);

    for (int i = 0; i < config.sample_count; ++i) {
        std::string sname =
            kMaterials[static_cast<size_t>(mat_pick(rng))]   + " " +
            kWaferTypes[static_cast<size_t>(type_pick(rng))] + " - " +
            kSizes[static_cast<size_t>(size_pick(rng))];
        std::string sid = fmt_sample_id(i + 1);

        // avg_production_time: 70% in [0.1, 1.0), 30% in [1.0, 2.0], 1 decimal place
        double pt_raw = (pt_selector(rng) < 0.7) ? pt_low(rng) : pt_high(rng);
        double avg_pt = std::round(pt_raw * 10.0) / 10.0;

        // yield_rate: normal(0.90, 0.07), clamped [0.70, 0.99], 2 decimal places
        double yr = std::round(std::clamp(yield_nd(rng), 0.70, 0.99) * 100.0) / 100.0;

        JsonValue rec = JsonValue::object();
        rec["sample_id"]           = JsonValue(sid);
        rec["sample_name"]         = JsonValue(sname);
        rec["avg_production_time"] = JsonValue(avg_pt);
        rec["yield_rate"]          = JsonValue(yr);
        rec["current_stock"]       = JsonValue(static_cast<int64_t>(stock_dist(rng)));
        sample_store.create(rec);

        samples.push_back({sid, sname, yr, avg_pt});
    }

    // ── Orders ───────────────────────────────────────────────────────────────
    DataStore order_store((fs::path(config.output_dir) / "orders.json").string());
    std::vector<OrderInfo> orders;
    orders.reserve(static_cast<size_t>(config.order_count));

    std::uniform_int_distribution<int> sample_pick(0, static_cast<int>(samples.size()) - 1);
    std::uniform_int_distribution<int> qty_dist(10, 500);
    std::uniform_int_distribution<int> status_dist(0, 4);
    std::uniform_int_distribution<int> customer_pick(0, static_cast<int>(kCustomerNames.size()) - 1);

    for (int i = 0; i < config.order_count; ++i) {
        const SampleInfo& s = samples[static_cast<size_t>(sample_pick(rng))];
        std::string onum = fmt_order_number(i + 1);
        int64_t qty = static_cast<int64_t>(qty_dist(rng));

        JsonValue rec = JsonValue::object();
        rec["order_number"]   = JsonValue(onum);
        rec["sample_id"]      = JsonValue(s.sample_id);
        rec["customer_name"]  = JsonValue(kCustomerNames[static_cast<size_t>(customer_pick(rng))]);
        rec["order_quantity"] = JsonValue(qty);
        rec["order_status"]   = JsonValue(static_cast<int64_t>(status_dist(rng)));
        order_store.create(rec);

        orders.push_back({onum, s.sample_name, qty, s.yield_rate, s.avg_production_time});
    }

    // ── Productions ──────────────────────────────────────────────────────────
    DataStore production_store((fs::path(config.output_dir) / "productions.json").string());

    std::uniform_int_distribution<int> order_pick(0, static_cast<int>(orders.size()) - 1);
    std::uniform_int_distribution<int> offset_dist(0, 180 * 24 * 3600);

    for (int i = 0; i < config.production_count; ++i) {
        const OrderInfo& o = orders[static_cast<size_t>(order_pick(rng))];

        std::uniform_int_distribution<int64_t> shortage_dist(0LL, o.order_quantity);
        int64_t shortage = shortage_dist(rng);

        // actual_production = ceil(shortage / (yield_rate * 0.9))
        int64_t actual_production = 0;
        if (shortage > 0) {
            actual_production = static_cast<int64_t>(
                std::ceil(static_cast<double>(shortage) / (o.yield_rate * 0.9))
            );
        }

        int64_t ordered_epoch = BASE_EPOCH + offset_dist(rng);

        // estimated_completion = actual_production * avg_production_time (총 생산 소요 시간, HH:MM)
        double total_minutes = static_cast<double>(actual_production) * o.avg_production_time;

        JsonValue rec = JsonValue::object();
        rec["order_number"]         = JsonValue(o.order_number);
        rec["sample_name"]          = JsonValue(o.sample_name);
        rec["order_quantity"]       = JsonValue(o.order_quantity);
        rec["shortage"]             = JsonValue(shortage);
        rec["actual_production"]    = JsonValue(actual_production);
        rec["ordered_at"]           = JsonValue(format_epoch(ordered_epoch));
        rec["estimated_completion"] = JsonValue(format_duration_hhmm(total_minutes));
        production_store.create(rec);
    }
}
