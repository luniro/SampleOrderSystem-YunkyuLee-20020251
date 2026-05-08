#include <gtest/gtest.h>
#include "data_store.hpp"
#include <filesystem>
#include <string>
#include <fstream>

namespace fs = std::filesystem;

// Helper: create a unique temp file path under the system temp dir
static std::string make_temp_path(const std::string& name) {
    auto tmp = fs::temp_directory_path() / name;
    return tmp.string();
}

// Helper: remove file if exists
static void remove_file(const std::string& path) {
    std::error_code ec;
    fs::remove(path, ec);
}

// ========== ORD record builders ==========

static JsonValue make_ord_a() {
    JsonValue rec = JsonValue::object();
    rec["order_number"]  = JsonValue(std::string("ORD-20240501-001"));
    rec["sample_id"]     = JsonValue(std::string("SMP-001"));
    rec["customer_name"] = JsonValue(std::string("홍길동"));
    rec["order_quantity"]= JsonValue(int64_t(50));
    rec["order_status"]  = JsonValue(int64_t(2));
    rec["approved_at"]   = JsonValue(std::string("2024-05-01 10:30:00"));
    rec["released_at"]   = JsonValue(nullptr);
    return rec;
}

static JsonValue make_ord_b() {
    JsonValue rec = JsonValue::object();
    rec["order_number"]  = JsonValue(std::string("ORD-20240501-002"));
    rec["sample_id"]     = JsonValue(std::string("SMP-001"));
    rec["customer_name"] = JsonValue(std::string("김철수"));
    rec["order_quantity"]= JsonValue(int64_t(30));
    rec["order_status"]  = JsonValue(int64_t(0));
    rec["approved_at"]   = JsonValue(nullptr);
    rec["released_at"]   = JsonValue(nullptr);
    return rec;
}

static JsonValue make_ord_c() {
    JsonValue rec = JsonValue::object();
    rec["order_number"]  = JsonValue(std::string("ORD-20240501-003"));
    rec["sample_id"]     = JsonValue(std::string("SMP-002"));
    rec["customer_name"] = JsonValue(std::string("이영희"));
    rec["order_quantity"]= JsonValue(int64_t(20));
    rec["order_status"]  = JsonValue(int64_t(4));
    rec["approved_at"]   = JsonValue(std::string("2024-05-01 09:00:00"));
    rec["released_at"]   = JsonValue(std::string("2024-05-02 14:00:00"));
    return rec;
}

static JsonValue make_prd_a() {
    JsonValue rec = JsonValue::object();
    rec["order_number"]         = JsonValue(std::string("ORD-20240501-001"));
    rec["sample_name"]          = JsonValue(std::string("산화철 나노입자"));
    rec["order_quantity"]       = JsonValue(int64_t(50));
    rec["shortage"]             = JsonValue(int64_t(20));
    rec["actual_production"]    = JsonValue(int64_t(23));
    rec["ordered_at"]           = JsonValue(std::string("2024-05-01 08:00:00"));
    rec["estimated_completion"] = JsonValue(std::string("03:30"));
    rec["production_start_at"]  = JsonValue(std::string("2024-05-01 10:30:00"));
    return rec;
}

static JsonValue make_prd_b() {
    JsonValue rec = JsonValue::object();
    rec["order_number"]         = JsonValue(std::string("ORD-20240501-004"));
    rec["sample_name"]          = JsonValue(std::string("탄소 나노튜브"));
    rec["order_quantity"]       = JsonValue(int64_t(100));
    rec["shortage"]             = JsonValue(int64_t(100));
    rec["actual_production"]    = JsonValue(int64_t(112));
    rec["ordered_at"]           = JsonValue(std::string("2024-05-01 11:00:00"));
    rec["estimated_completion"] = JsonValue(std::string("06:00"));
    rec["production_start_at"]  = JsonValue(nullptr);
    return rec;
}

// ========== TC-DS-01 ==========
// null 필드 포함 레코드 create 후 동일 id로 read 왕복
TEST(TG_DS, TC_DS_01_NullFieldsCreateRead) {
    std::string path = make_temp_path("tc_ds_01_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_b());
    JsonValue result = ds.read(id);

    EXPECT_TRUE(result["approved_at"].is_null());
    EXPECT_TRUE(result["released_at"].is_null());
    EXPECT_EQ(result["order_number"].as_string(), "ORD-20240501-002");

    remove_file(path);
}

// ========== TC-DS-02 ==========
// string 필드와 null 필드가 혼재하는 레코드 create → read 왕복
TEST(TG_DS, TC_DS_02_MixedFieldsCreateRead) {
    std::string path = make_temp_path("tc_ds_02_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_a());
    JsonValue result = ds.read(id);

    EXPECT_EQ(result["approved_at"].as_string(), "2024-05-01 10:30:00");
    EXPECT_TRUE(result["released_at"].is_null());

    remove_file(path);
}

// ========== TC-DS-03 ==========
// null 필드를 string으로 update 후 read (null→string)
TEST(TG_DS, TC_DS_03_NullToStringUpdate) {
    std::string path = make_temp_path("tc_ds_03_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_b());

    JsonValue patch = JsonValue::object();
    patch["approved_at"] = JsonValue(std::string("2024-05-01 12:00:00"));
    ds.update(id, patch);

    JsonValue result = ds.read(id);
    EXPECT_FALSE(result["approved_at"].is_null());
    EXPECT_EQ(result["approved_at"].as_string(), "2024-05-01 12:00:00");

    remove_file(path);
}

// ========== TC-DS-04 ==========
// string 필드를 null로 update 후 read (string→null)
TEST(TG_DS, TC_DS_04_StringToNullUpdate) {
    std::string path = make_temp_path("tc_ds_04_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_a());

    JsonValue patch = JsonValue::object();
    patch["approved_at"] = JsonValue(nullptr);
    ds.update(id, patch);

    JsonValue result = ds.read(id);
    EXPECT_TRUE(result["approved_at"].is_null());

    remove_file(path);
}

// ========== TC-DS-05 ==========
// released_at: null→string update 후 read
TEST(TG_DS, TC_DS_05_ReleasedAtNullToString) {
    std::string path = make_temp_path("tc_ds_05_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_a()); // released_at=null

    JsonValue patch = JsonValue::object();
    patch["released_at"] = JsonValue(std::string("2024-05-03 09:00:00"));
    ds.update(id, patch);

    JsonValue result = ds.read(id);
    EXPECT_FALSE(result["released_at"].is_null());
    EXPECT_EQ(result["released_at"].as_string(), "2024-05-03 09:00:00");

    remove_file(path);
}

// ========== TC-DS-06 ==========
// released_at: string→null update 후 read
TEST(TG_DS, TC_DS_06_ReleasedAtStringToNull) {
    std::string path = make_temp_path("tc_ds_06_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_c()); // released_at="2024-05-02 14:00:00"

    JsonValue patch = JsonValue::object();
    patch["released_at"] = JsonValue(nullptr);
    ds.update(id, patch);

    JsonValue result = ds.read(id);
    EXPECT_TRUE(result["released_at"].is_null());

    remove_file(path);
}

// ========== TC-DS-07 ==========
// production_start_at null 포함 레코드 create → read 왕복
TEST(TG_DS, TC_DS_07_ProductionStartAtNullCreateRead) {
    std::string path = make_temp_path("tc_ds_07_productions.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_prd_b());
    JsonValue result = ds.read(id);

    EXPECT_TRUE(result["production_start_at"].is_null());
    EXPECT_EQ(result["order_number"].as_string(), "ORD-20240501-004");

    remove_file(path);
}

// ========== TC-DS-08 ==========
// production_start_at: null→string update 후 read
TEST(TG_DS, TC_DS_08_ProductionStartAtNullToString) {
    std::string path = make_temp_path("tc_ds_08_productions.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_prd_b()); // production_start_at=null

    JsonValue patch = JsonValue::object();
    patch["production_start_at"] = JsonValue(std::string("2024-05-01 14:00:00"));
    ds.update(id, patch);

    JsonValue result = ds.read(id);
    EXPECT_FALSE(result["production_start_at"].is_null());
    EXPECT_EQ(result["production_start_at"].as_string(), "2024-05-01 14:00:00");

    remove_file(path);
}

// ========== TC-DS-09 ==========
// null 필드 포함 레코드 파일 저장 후 DataStore 재초기화(load) — 값 보존 검증
TEST(TG_DS, TC_DS_09_NullFieldPersistedAfterReload) {
    std::string path = make_temp_path("tc_ds_09_orders.json");
    remove_file(path);

    int64_t id;
    {
        DataStore ds(path);
        id = ds.create(make_ord_b()); // approved_at=null, released_at=null
    }

    // Reload
    DataStore ds2(path);
    JsonValue result = ds2.read(id);

    EXPECT_TRUE(result["approved_at"].is_null());
    EXPECT_TRUE(result["released_at"].is_null());
    EXPECT_EQ(result["order_number"].as_string(), "ORD-20240501-002");

    // Verify JSON file contains null literals
    std::ifstream f(path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("null"), std::string::npos);

    remove_file(path);
}

// ========== TC-DS-10 ==========
// string 필드 포함 레코드 파일 저장 후 재초기화 — 값 보존 검증
TEST(TG_DS, TC_DS_10_StringFieldPersistedAfterReload) {
    std::string path = make_temp_path("tc_ds_10_orders.json");
    remove_file(path);

    int64_t id;
    {
        DataStore ds(path);
        id = ds.create(make_ord_a()); // approved_at=string, released_at=null
    }

    DataStore ds2(path);
    JsonValue result = ds2.read(id);

    EXPECT_EQ(result["approved_at"].as_string(), "2024-05-01 10:30:00");
    EXPECT_TRUE(result["released_at"].is_null());

    remove_file(path);
}

// ========== TC-DS-11 ==========
// null 필드 포함 레코드를 update 후 파일 저장 → 재초기화 → read — 값 보존 검증
TEST(TG_DS, TC_DS_11_UpdatedFieldPersistedAfterReload) {
    std::string path = make_temp_path("tc_ds_11_orders.json");
    remove_file(path);

    int64_t id;
    {
        DataStore ds(path);
        id = ds.create(make_ord_b()); // approved_at=null
        JsonValue patch = JsonValue::object();
        patch["approved_at"] = JsonValue(std::string("2024-05-01 12:00:00"));
        ds.update(id, patch);
    }

    DataStore ds2(path);
    JsonValue result = ds2.read(id);
    EXPECT_EQ(result["approved_at"].as_string(), "2024-05-01 12:00:00");

    remove_file(path);
}

// ========== TC-DS-12 ==========
// read_all 결과에 null 필드 포함 레코드 포함 여부 확인
TEST(TG_DS, TC_DS_12_ReadAllContainsNullRecord) {
    std::string path = make_temp_path("tc_ds_12_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id_a = ds.create(make_ord_a()); // approved_at=string
    int64_t id_b = ds.create(make_ord_b()); // approved_at=null
    int64_t id_c = ds.create(make_ord_c()); // released_at=string

    auto all = ds.read_all();
    EXPECT_EQ(all.size(), size_t(3));

    // Find ORD-B by id
    bool found_ord_b_null = false;
    for (const auto& rec : all) {
        if (rec["id"].as_integer() == id_b) {
            EXPECT_TRUE(rec["approved_at"].is_null());
            found_ord_b_null = true;
        }
    }
    EXPECT_TRUE(found_ord_b_null);

    // All three records present
    bool found_a = false, found_b = false, found_c = false;
    for (const auto& rec : all) {
        int64_t id = rec["id"].as_integer();
        if (id == id_a) found_a = true;
        if (id == id_b) found_b = true;
        if (id == id_c) found_c = true;
    }
    EXPECT_TRUE(found_a);
    EXPECT_TRUE(found_b);
    EXPECT_TRUE(found_c);

    remove_file(path);
}

// ========== TC-DS-13 ==========
// read_all 결과에서 null 필드 값 정확성 확인
TEST(TG_DS, TC_DS_13_ReadAllNullFieldAccuracy) {
    std::string path = make_temp_path("tc_ds_13_orders.json");
    remove_file(path);

    DataStore ds(path);
    ds.create(make_ord_a()); // approved_at="2024-05-01 10:30:00", released_at=null

    auto all = ds.read_all();
    ASSERT_EQ(all.size(), size_t(1));

    EXPECT_EQ(all[0]["approved_at"].as_string(), "2024-05-01 10:30:00");
    EXPECT_TRUE(all[0]["released_at"].is_null());

    remove_file(path);
}

// ========== TC-DS-14 ==========
// null 필드 포함 레코드 remove 후 read_all에서 제거 확인
TEST(TG_DS, TC_DS_14_RemoveNullRecordFromReadAll) {
    std::string path = make_temp_path("tc_ds_14_orders.json");
    remove_file(path);

    DataStore ds(path);
    ds.create(make_ord_a());
    int64_t id_b = ds.create(make_ord_b()); // approved_at=null

    ds.remove(id_b);

    auto all = ds.read_all();
    EXPECT_EQ(all.size(), size_t(1));

    for (const auto& rec : all) {
        EXPECT_NE(rec["id"].as_integer(), id_b);
    }

    remove_file(path);
}

// ========== TC-DS-15 ==========
// null 필드 포함 레코드 remove 후 동일 id로 read 시 예외
TEST(TG_DS, TC_DS_15_ReadAfterRemoveThrows) {
    std::string path = make_temp_path("tc_ds_15_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_b()); // approved_at=null
    ds.remove(id);

    EXPECT_THROW(ds.read(id), RecordNotFoundError);

    remove_file(path);
}

// ========== TC-DS-16 ==========
// 존재하지 않는 id로 read 시 RecordNotFoundError
TEST(TG_DS, TC_DS_16_ReadNonExistentIdThrows) {
    std::string path = make_temp_path("tc_ds_16_orders.json");
    remove_file(path);

    DataStore ds(path);

    try {
        ds.read(999);
        FAIL() << "Expected RecordNotFoundError";
    } catch (const RecordNotFoundError& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("999"), std::string::npos);
    }

    remove_file(path);
}

// ========== TC-DS-17 ==========
// 존재하지 않는 id로 update 시 RecordNotFoundError
TEST(TG_DS, TC_DS_17_UpdateNonExistentIdThrows) {
    std::string path = make_temp_path("tc_ds_17_orders.json");
    remove_file(path);

    DataStore ds(path);

    JsonValue patch = JsonValue::object();
    patch["approved_at"] = JsonValue(std::string("2024-05-01 10:30:00"));

    try {
        ds.update(999, patch);
        FAIL() << "Expected RecordNotFoundError";
    } catch (const RecordNotFoundError& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("999"), std::string::npos);
    }

    remove_file(path);
}

// ========== TC-DS-18 ==========
// 존재하지 않는 id로 remove 시 RecordNotFoundError
TEST(TG_DS, TC_DS_18_RemoveNonExistentIdThrows) {
    std::string path = make_temp_path("tc_ds_18_orders.json");
    remove_file(path);

    DataStore ds(path);

    try {
        ds.remove(999);
        FAIL() << "Expected RecordNotFoundError";
    } catch (const RecordNotFoundError& e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("999"), std::string::npos);
    }

    remove_file(path);
}

// ========== TC-DS-19 ==========
// 레코드가 1건 있을 때 존재하지 않는 id로 read
TEST(TG_DS, TC_DS_19_ReadNonExistentWhenOneRecordExists) {
    std::string path = make_temp_path("tc_ds_19_orders.json");
    remove_file(path);

    DataStore ds(path);
    ds.create(make_ord_a()); // id=1

    EXPECT_THROW(ds.read(2), RecordNotFoundError);

    remove_file(path);
}

// ========== TC-DS-20 ==========
// 레코드가 1건 있을 때 존재하지 않는 id로 update
TEST(TG_DS, TC_DS_20_UpdateNonExistentWhenOneRecordExists) {
    std::string path = make_temp_path("tc_ds_20_orders.json");
    remove_file(path);

    DataStore ds(path);
    ds.create(make_ord_a()); // id=1

    JsonValue patch = JsonValue::object();
    patch["approved_at"] = JsonValue(nullptr);

    EXPECT_THROW(ds.update(2, patch), RecordNotFoundError);

    remove_file(path);
}

// ========== TC-DS-21 ==========
// 레코드가 1건 있을 때 존재하지 않는 id로 remove
TEST(TG_DS, TC_DS_21_RemoveNonExistentWhenOneRecordExists) {
    std::string path = make_temp_path("tc_ds_21_orders.json");
    remove_file(path);

    DataStore ds(path);
    ds.create(make_ord_a()); // id=1

    EXPECT_THROW(ds.remove(2), RecordNotFoundError);

    remove_file(path);
}

// ========== TC-DS-22 ==========
// update 시 id 필드는 변경 불가
TEST(TG_DS, TC_DS_22_IdFieldImmutableOnUpdate) {
    std::string path = make_temp_path("tc_ds_22_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_a()); // id=1

    JsonValue patch = JsonValue::object();
    patch["id"]          = JsonValue(int64_t(999));
    patch["approved_at"] = JsonValue(std::string("2024-05-01 11:00:00"));
    ds.update(id, patch);

    JsonValue result = ds.read(id); // read(1) must succeed
    EXPECT_EQ(result["id"].as_integer(), id);
    EXPECT_EQ(result["approved_at"].as_string(), "2024-05-01 11:00:00");

    remove_file(path);
}

// ========== TC-DS-23 ==========
// 빈 스토어에서 read_all 호출 시 빈 벡터 반환
TEST(TG_DS, TC_DS_23_ReadAllEmptyStore) {
    std::string path = make_temp_path("tc_ds_23_orders.json");
    remove_file(path);

    DataStore ds(path);
    auto all = ds.read_all();

    EXPECT_EQ(all.size(), size_t(0));

    remove_file(path);
}

// ========== TC-DS-24 ==========
// null 필드 포함 레코드에 대해 update로 추가 필드 삽입
TEST(TG_DS, TC_DS_24_UpdatePreservesExistingNullFields) {
    std::string path = make_temp_path("tc_ds_24_orders.json");
    remove_file(path);

    DataStore ds(path);
    int64_t id = ds.create(make_ord_b()); // approved_at=null, released_at=null, order_status=0

    JsonValue patch = JsonValue::object();
    patch["order_status"] = JsonValue(int64_t(1));
    ds.update(id, patch);

    JsonValue result = ds.read(id);
    EXPECT_EQ(result["order_status"].as_integer(), int64_t(1));
    EXPECT_TRUE(result["approved_at"].is_null());

    remove_file(path);
}

// ========== TC-DS-25 ==========
// production_start_at: string→null update 후 파일 저장·재로드 — null 보존
TEST(TG_DS, TC_DS_25_ProductionStartAtStringToNullPersisted) {
    std::string path = make_temp_path("tc_ds_25_productions.json");
    remove_file(path);

    int64_t id;
    {
        DataStore ds(path);
        id = ds.create(make_prd_a()); // production_start_at="2024-05-01 10:30:00"

        JsonValue patch = JsonValue::object();
        patch["production_start_at"] = JsonValue(nullptr);
        ds.update(id, patch);
    }

    // Reload
    DataStore ds2(path);
    JsonValue result = ds2.read(id);
    EXPECT_TRUE(result["production_start_at"].is_null());

    // Verify null literal in file
    std::ifstream f(path);
    std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("null"), std::string::npos);

    remove_file(path);
}
