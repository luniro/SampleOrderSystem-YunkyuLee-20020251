#include "domain/types.hpp"

Sample Sample::from_json(const JsonValue& v) {
    return {
        v["id"].as_integer(),
        v["sample_id"].as_string(),
        v["sample_name"].as_string(),
        v["avg_production_time"].as_float(),
        v["yield_rate"].as_float(),
        v["current_stock"].as_integer()
    };
}

Order Order::from_json(const JsonValue& v) {
    const auto& aa = v["approved_at"];
    const auto& ra = v["released_at"];
    return {
        v["id"].as_integer(),
        v["order_number"].as_string(),
        v["sample_id"].as_string(),
        v["customer_name"].as_string(),
        v["order_quantity"].as_integer(),
        static_cast<int>(v["order_status"].as_integer()),
        aa.is_null() ? std::string{} : aa.as_string(),
        ra.is_null() ? std::string{} : ra.as_string()
    };
}

Production Production::from_json(const JsonValue& v) {
    return {
        v["id"].as_integer(),
        v["order_number"].as_string(),
        v["sample_name"].as_string(),
        v["order_quantity"].as_integer(),
        v["shortage"].as_integer(),
        v["actual_production"].as_integer(),
        v["ordered_at"].as_string(),
        v["estimated_completion"].as_string(),
        v["production_start_at"].as_string()
    };
}
