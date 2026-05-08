#pragma once
#include "json/json.hpp"
#include <stdexcept>
#include <string>
#include <vector>

class RecordNotFoundError : public std::runtime_error {
public:
    explicit RecordNotFoundError(int64_t id)
        : std::runtime_error("record not found: id=" + std::to_string(id)) {}
};

class DataStore {
public:
    explicit DataStore(const std::string& file_path);

    int64_t                create(JsonValue record);
    JsonValue              read(int64_t id) const;
    std::vector<JsonValue> read_all() const;
    void                   update(int64_t id, const JsonValue& fields);
    void                   remove(int64_t id);

private:
    void load();
    void save() const;

    std::string path_;
    JsonValue   data_;
};
