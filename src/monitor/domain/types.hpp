#pragma once
#include "json/json.hpp"
#include <string>

struct Sample {
    int64_t     id;
    std::string sample_id;
    std::string sample_name;
    double      avg_production_time;
    double      yield_rate;
    int64_t     current_stock;

    static Sample from_json(const JsonValue& v);
};

struct Order {
    int64_t     id;
    std::string order_number;
    std::string sample_id;
    std::string customer_name;
    int64_t     order_quantity;
    int         order_status;
    std::string approved_at;
    std::string released_at;

    static constexpr int STATUS_RESERVED  = 0;
    static constexpr int STATUS_REJECTED  = 1;
    static constexpr int STATUS_PRODUCING = 2;
    static constexpr int STATUS_CONFIRMED = 3;
    static constexpr int STATUS_RELEASED  = 4;

    static Order from_json(const JsonValue& v);
};

struct Production {
    int64_t     id;
    std::string order_number;
    std::string sample_name;
    int64_t     order_quantity;
    int64_t     shortage;
    int64_t     actual_production;
    std::string ordered_at;
    std::string estimated_completion;
    std::string production_start_at;

    static Production from_json(const JsonValue& v);
};
