#pragma once
#include "domain/types.hpp"
#include "data_store.hpp"
#include <optional>
#include <string>
#include <vector>

class OrderRepository {
public:
    explicit OrderRepository(const std::string& file_path);

    std::vector<Order>   find_all();
    std::vector<Order>   find_by_status(int status_code);
    std::optional<Order> find_by_order_number(const std::string& order_number);
    void                 refresh();

private:
    std::string file_path_;
    DataStore   store_;
};
