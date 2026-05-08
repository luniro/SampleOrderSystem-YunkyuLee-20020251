#include "repository/production_repository.hpp"

ProductionRepository::ProductionRepository(const std::string& file_path)
    : file_path_(file_path), store_(file_path) {}

void ProductionRepository::refresh() {
    store_ = DataStore(file_path_);
}

std::vector<Production> ProductionRepository::find_all() {
    auto records = store_.read_all();
    std::vector<Production> result;
    result.reserve(records.size());
    for (const auto& rec : records)
        result.push_back(Production::from_json(rec));
    return result;
}

std::optional<Production> ProductionRepository::find_by_order_number(const std::string& order_number) {
    for (const auto& rec : store_.read_all()) {
        if (rec["order_number"].as_string() == order_number)
            return Production::from_json(rec);
    }
    return std::nullopt;
}
