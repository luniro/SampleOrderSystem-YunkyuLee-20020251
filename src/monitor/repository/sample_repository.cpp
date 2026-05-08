#include "repository/sample_repository.hpp"

SampleRepository::SampleRepository(const std::string& file_path)
    : file_path_(file_path), store_(file_path) {}

void SampleRepository::refresh() {
    store_ = DataStore(file_path_);
}

std::vector<Sample> SampleRepository::find_all() {
    auto records = store_.read_all();
    std::vector<Sample> result;
    result.reserve(records.size());
    for (const auto& rec : records)
        result.push_back(Sample::from_json(rec));
    return result;
}

std::optional<Sample> SampleRepository::find_by_sample_id(const std::string& sample_id) {
    for (const auto& rec : store_.read_all()) {
        if (rec["sample_id"].as_string() == sample_id)
            return Sample::from_json(rec);
    }
    return std::nullopt;
}
