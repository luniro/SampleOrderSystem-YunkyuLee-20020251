#pragma once
#include "domain/types.hpp"
#include "data_store.hpp"
#include <optional>
#include <string>
#include <vector>

class SampleRepository {
public:
    explicit SampleRepository(const std::string& file_path);

    std::vector<Sample>   find_all();
    std::optional<Sample> find_by_sample_id(const std::string& sample_id);
    void                  refresh();

private:
    std::string file_path_;
    DataStore   store_;
};
