#pragma once
#include "domain/types.hpp"
#include "data_store.hpp"
#include <optional>
#include <string>
#include <vector>

class ProductionRepository {
public:
    explicit ProductionRepository(const std::string& file_path);

    std::vector<Production>   find_all();
    std::optional<Production> find_by_order_number(const std::string& order_number);
    void                      refresh();

private:
    std::string file_path_;
    DataStore   store_;
};
