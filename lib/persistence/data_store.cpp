#include "data_store.hpp"

DataStore::DataStore(const std::string& file_path) : path_(file_path) {
    load();
}

void DataStore::load() {
    try {
        data_ = Json::parse_file(path_);
    } catch (const JsonIOError&) {
        data_ = JsonValue::object();
        data_["next_id"] = JsonValue(int64_t(1));
        data_["records"] = JsonValue::array();
    }
}

void DataStore::save() const {
    SerializeOptions opts;
    opts.pretty = true;
    Json::dump_file(data_, path_, opts);
}

int64_t DataStore::create(JsonValue record) {
    int64_t id = data_["next_id"].as_integer();
    record["id"] = JsonValue(id);
    data_["next_id"] = JsonValue(id + 1);
    data_["records"].push_back(std::move(record));
    save();
    return id;
}

JsonValue DataStore::read(int64_t id) const {
    const auto& records = data_["records"].as_array();
    for (const auto& rec : records) {
        if (rec["id"].as_integer() == id)
            return rec;
    }
    throw RecordNotFoundError(id);
}

std::vector<JsonValue> DataStore::read_all() const {
    const auto& records = data_["records"].as_array();
    return std::vector<JsonValue>(records.begin(), records.end());
}

void DataStore::update(int64_t id, const JsonValue& fields) {
    auto& records = data_["records"].as_array();
    for (auto& rec : records) {
        if (rec["id"].as_integer() == id) {
            for (const auto& [key, val] : fields.as_object()) {
                if (key != "id")
                    rec[key] = val;
            }
            save();
            return;
        }
    }
    throw RecordNotFoundError(id);
}

void DataStore::remove(int64_t id) {
    auto& records = data_["records"].as_array();
    for (auto it = records.begin(); it != records.end(); ++it) {
        if ((*it)["id"].as_integer() == id) {
            records.erase(it);
            save();
            return;
        }
    }
    throw RecordNotFoundError(id);
}
