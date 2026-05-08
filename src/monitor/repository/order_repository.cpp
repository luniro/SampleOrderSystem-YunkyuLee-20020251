#include "repository/order_repository.hpp"

OrderRepository::OrderRepository(const std::string& file_path)
    : file_path_(file_path), store_(file_path) {}

void OrderRepository::refresh() {
    store_ = DataStore(file_path_);
}

std::vector<Order> OrderRepository::find_all() {
    auto records = store_.read_all();
    std::vector<Order> result;
    result.reserve(records.size());
    for (const auto& rec : records)
        result.push_back(Order::from_json(rec));
    return result;
}

std::vector<Order> OrderRepository::find_by_status(int status_code) {
    std::vector<Order> result;
    for (const auto& rec : store_.read_all()) {
        if (static_cast<int>(rec["order_status"].as_integer()) == status_code)
            result.push_back(Order::from_json(rec));
    }
    return result;
}

std::optional<Order> OrderRepository::find_by_order_number(const std::string& order_number) {
    for (const auto& rec : store_.read_all()) {
        if (rec["order_number"].as_string() == order_number)
            return Order::from_json(rec);
    }
    return std::nullopt;
}
