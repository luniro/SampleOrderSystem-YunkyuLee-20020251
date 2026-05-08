#include <gtest/gtest.h>
#include "monitor/domain/types.hpp"
#include "monitor/repository/order_repository.hpp"
#include "monitor/repository/production_repository.hpp"
#include "monitor/repository/sample_repository.hpp"
#include "data_store.hpp"
#include "json/json.hpp"

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string make_temp_path(const std::string& name) {
    auto tmp = fs::temp_directory_path() / name;
    return tmp.string();
}

static void remove_file(const std::string& path) {
    std::error_code ec;
    fs::remove(path, ec);
}

// ── Record Builders ───────────────────────────────────────────────────────────

// ORD-A: Producing, approved_at=string, released_at=null
static JsonValue make_ord_a_json() {
    JsonValue rec = JsonValue::object();
    rec["id"]            = JsonValue(int64_t(1));
    rec["order_number"]  = JsonValue(std::string("ORD-20240501-001"));
    rec["sample_id"]     = JsonValue(std::string("SMP-001"));
    rec["customer_name"] = JsonValue(std::string("홍길동"));
    rec["order_quantity"]= JsonValue(int64_t(50));
    rec["order_status"]  = JsonValue(int64_t(2));
    rec["approved_at"]   = JsonValue(std::string("2024-05-01 10:30:00"));
    rec["released_at"]   = JsonValue(nullptr);
    return rec;
}

// ORD-B: Reserved, approved_at=null, released_at=null
static JsonValue make_ord_b_json() {
    JsonValue rec = JsonValue::object();
    rec["id"]            = JsonValue(int64_t(2));
    rec["order_number"]  = JsonValue(std::string("ORD-20240501-002"));
    rec["sample_id"]     = JsonValue(std::string("SMP-001"));
    rec["customer_name"] = JsonValue(std::string("김철수"));
    rec["order_quantity"]= JsonValue(int64_t(30));
    rec["order_status"]  = JsonValue(int64_t(0));
    rec["approved_at"]   = JsonValue(nullptr);
    rec["released_at"]   = JsonValue(nullptr);
    return rec;
}

// ORD-C: Released, approved_at=string, released_at=string
static JsonValue make_ord_c_json() {
    JsonValue rec = JsonValue::object();
    rec["id"]            = JsonValue(int64_t(3));
    rec["order_number"]  = JsonValue(std::string("ORD-20240501-003"));
    rec["sample_id"]     = JsonValue(std::string("SMP-002"));
    rec["customer_name"] = JsonValue(std::string("이영희"));
    rec["order_quantity"]= JsonValue(int64_t(20));
    rec["order_status"]  = JsonValue(int64_t(4));
    rec["approved_at"]   = JsonValue(std::string("2024-05-01 09:00:00"));
    rec["released_at"]   = JsonValue(std::string("2024-05-02 14:00:00"));
    return rec;
}

// PRD-A: production_start_at=string
static JsonValue make_prd_a_json() {
    JsonValue rec = JsonValue::object();
    rec["id"]                    = JsonValue(int64_t(1));
    rec["order_number"]          = JsonValue(std::string("ORD-20240501-001"));
    rec["sample_name"]           = JsonValue(std::string("산화철 나노입자"));
    rec["order_quantity"]        = JsonValue(int64_t(50));
    rec["shortage"]              = JsonValue(int64_t(20));
    rec["actual_production"]     = JsonValue(int64_t(23));
    rec["ordered_at"]            = JsonValue(std::string("2024-05-01 08:00:00"));
    rec["estimated_completion"]  = JsonValue(std::string("03:30"));
    rec["production_start_at"]   = JsonValue(std::string("2024-05-01 10:30:00"));
    return rec;
}

// Sample record
static JsonValue make_sample_json() {
    JsonValue rec = JsonValue::object();
    rec["id"]                    = JsonValue(int64_t(1));
    rec["sample_id"]             = JsonValue(std::string("SMP-001"));
    rec["sample_name"]           = JsonValue(std::string("산화철 나노입자"));
    rec["avg_production_time"]   = JsonValue(4.5);
    rec["yield_rate"]            = JsonValue(0.87);
    rec["current_stock"]         = JsonValue(int64_t(200));
    return rec;
}

// ── TC-MT-01: Sample::from_json — 모든 필드 정상 역직렬화 ────────────────────
TEST(TG_MT, TC_MT_01_SampleFromJsonAllFields) {
    JsonValue v = make_sample_json();
    Sample s = Sample::from_json(v);

    EXPECT_EQ(s.id, int64_t(1));
    EXPECT_EQ(s.sample_id, "SMP-001");
    EXPECT_EQ(s.sample_name, "산화철 나노입자");
    EXPECT_DOUBLE_EQ(s.avg_production_time, 4.5);
    EXPECT_DOUBLE_EQ(s.yield_rate, 0.87);
    EXPECT_EQ(s.current_stock, int64_t(200));
}

// ── TC-MT-02: Order::from_json — approved_at=string, released_at=null (Producing) ──
TEST(TG_MT, TC_MT_02_OrderFromJsonApprovedAtStringReleasedAtNull) {
    JsonValue v = make_ord_a_json();
    Order o = Order::from_json(v);

    EXPECT_EQ(o.order_status, 2);
    EXPECT_EQ(o.approved_at, "2024-05-01 10:30:00");
    EXPECT_EQ(o.released_at, "");  // null -> empty string
}

// ── TC-MT-03: Order::from_json — approved_at=null, released_at=null (Reserved) ─
TEST(TG_MT, TC_MT_03_OrderFromJsonBothNullReserved) {
    JsonValue v = make_ord_b_json();
    Order o = Order::from_json(v);

    EXPECT_EQ(o.order_status, 0);
    EXPECT_EQ(o.approved_at, "");   // null -> empty string
    EXPECT_EQ(o.released_at, "");   // null -> empty string
}

// ── TC-MT-04: Order::from_json — approved_at=string, released_at=string (Released) ─
TEST(TG_MT, TC_MT_04_OrderFromJsonBothStringsReleased) {
    JsonValue v = make_ord_c_json();
    Order o = Order::from_json(v);

    EXPECT_EQ(o.order_status, 4);
    EXPECT_EQ(o.approved_at, "2024-05-01 09:00:00");
    EXPECT_EQ(o.released_at, "2024-05-02 14:00:00");
}

// ── TC-MT-05: Order::from_json — 기존 필드 정상 역직렬화 ──────────────────────
TEST(TG_MT, TC_MT_05_OrderFromJsonExistingFields) {
    JsonValue v = make_ord_a_json();
    // Override id to 1 explicitly from make_ord_a_json
    Order o = Order::from_json(v);

    EXPECT_EQ(o.id, int64_t(1));
    EXPECT_EQ(o.order_number, "ORD-20240501-001");
    EXPECT_EQ(o.sample_id, "SMP-001");
    EXPECT_EQ(o.customer_name, "홍길동");
    EXPECT_EQ(o.order_quantity, int64_t(50));
}

// ── TC-MT-06: Production::from_json — production_start_at=string ──────────────
TEST(TG_MT, TC_MT_06_ProductionFromJsonStartAt) {
    JsonValue v = make_prd_a_json();
    Production p = Production::from_json(v);

    EXPECT_EQ(p.production_start_at, "2024-05-01 10:30:00");
}

// ── TC-MT-07: Production::from_json — 모든 필드 정상 역직렬화 ─────────────────
TEST(TG_MT, TC_MT_07_ProductionFromJsonAllFields) {
    JsonValue v = make_prd_a_json();
    Production p = Production::from_json(v);

    EXPECT_EQ(p.id, int64_t(1));
    EXPECT_EQ(p.order_number, "ORD-20240501-001");
    EXPECT_EQ(p.sample_name, "산화철 나노입자");
    EXPECT_EQ(p.order_quantity, int64_t(50));
    EXPECT_EQ(p.shortage, int64_t(20));
    EXPECT_EQ(p.actual_production, int64_t(23));
    EXPECT_EQ(p.ordered_at, "2024-05-01 08:00:00");
    EXPECT_EQ(p.estimated_completion, "03:30");
    EXPECT_EQ(p.production_start_at, "2024-05-01 10:30:00");
}

// ── TC-MT-08: Order::STATUS_RESERVED == 0 ────────────────────────────────────
TEST(TG_MT, TC_MT_08_StatusReservedIs0) {
    EXPECT_EQ(Order::STATUS_RESERVED, 0);
}

// ── TC-MT-09: Order::STATUS_REJECTED == 1 ────────────────────────────────────
TEST(TG_MT, TC_MT_09_StatusRejectedIs1) {
    EXPECT_EQ(Order::STATUS_REJECTED, 1);
}

// ── TC-MT-10: Order::STATUS_PRODUCING == 2 ───────────────────────────────────
TEST(TG_MT, TC_MT_10_StatusProducingIs2) {
    EXPECT_EQ(Order::STATUS_PRODUCING, 2);
}

// ── TC-MT-11: Order::STATUS_CONFIRMED == 3 ───────────────────────────────────
TEST(TG_MT, TC_MT_11_StatusConfirmedIs3) {
    EXPECT_EQ(Order::STATUS_CONFIRMED, 3);
}

// ── TC-MT-12: Order::STATUS_RELEASED == 4 (RELEASE → RELEASED 오타 수정 확인) ─
TEST(TG_MT, TC_MT_12_StatusReleasedIs4) {
    // Verifies the enum was renamed from STATUS_RELEASE to STATUS_RELEASED
    // and the value is 4 as per DATA_SCHEMA.md
    EXPECT_EQ(Order::STATUS_RELEASED, 4);
}

// ── TC-MT-13: Order::from_json — id 필드 정상 역직렬화 ───────────────────────
TEST(TG_MT, TC_MT_13_OrderFromJsonId) {
    JsonValue v = make_ord_b_json();
    // make_ord_b_json has id=2
    Order o = Order::from_json(v);
    EXPECT_EQ(o.id, int64_t(2));
}

// ── TC-MT-14: OrderRepository::find_all ──────────────────────────────────────
TEST(TG_MT, TC_MT_14_OrderRepositoryFindAll) {
    std::string path = make_temp_path("tc_mt_14_orders.json");
    remove_file(path);

    {
        DataStore ds(path);
        // Remove hardcoded id from records before inserting (DataStore assigns ids)
        JsonValue ra = make_ord_a_json();
        // Remove the id key by creating fresh record without id
        JsonValue ra2 = JsonValue::object();
        ra2["order_number"]  = ra["order_number"];
        ra2["sample_id"]     = ra["sample_id"];
        ra2["customer_name"] = ra["customer_name"];
        ra2["order_quantity"]= ra["order_quantity"];
        ra2["order_status"]  = ra["order_status"];
        ra2["approved_at"]   = ra["approved_at"];
        ra2["released_at"]   = ra["released_at"];
        ds.create(ra2);

        JsonValue rb = make_ord_b_json();
        JsonValue rb2 = JsonValue::object();
        rb2["order_number"]  = rb["order_number"];
        rb2["sample_id"]     = rb["sample_id"];
        rb2["customer_name"] = rb["customer_name"];
        rb2["order_quantity"]= rb["order_quantity"];
        rb2["order_status"]  = rb["order_status"];
        rb2["approved_at"]   = rb["approved_at"];
        rb2["released_at"]   = rb["released_at"];
        ds.create(rb2);

        JsonValue rc = make_ord_c_json();
        JsonValue rc2 = JsonValue::object();
        rc2["order_number"]  = rc["order_number"];
        rc2["sample_id"]     = rc["sample_id"];
        rc2["customer_name"] = rc["customer_name"];
        rc2["order_quantity"]= rc["order_quantity"];
        rc2["order_status"]  = rc["order_status"];
        rc2["approved_at"]   = rc["approved_at"];
        rc2["released_at"]   = rc["released_at"];
        ds.create(rc2);
    }

    OrderRepository repo(path);
    auto all = repo.find_all();

    EXPECT_EQ(all.size(), size_t(3));

    // Check approved_at values
    bool found_a = false, found_b = false;
    for (const auto& o : all) {
        if (o.order_number == "ORD-20240501-001") {
            found_a = true;
            EXPECT_EQ(o.approved_at, "2024-05-01 10:30:00");
        }
        if (o.order_number == "ORD-20240501-002") {
            found_b = true;
            EXPECT_EQ(o.approved_at, "");  // null -> empty string
        }
    }
    EXPECT_TRUE(found_a);
    EXPECT_TRUE(found_b);

    remove_file(path);
}

// ── TC-MT-15: OrderRepository::find_by_status ────────────────────────────────
TEST(TG_MT, TC_MT_15_OrderRepositoryFindByStatus) {
    std::string path = make_temp_path("tc_mt_15_orders.json");
    remove_file(path);

    {
        DataStore ds(path);
        auto make_minimal = [](const JsonValue& src) {
            JsonValue r = JsonValue::object();
            r["order_number"]  = src["order_number"];
            r["sample_id"]     = src["sample_id"];
            r["customer_name"] = src["customer_name"];
            r["order_quantity"]= src["order_quantity"];
            r["order_status"]  = src["order_status"];
            r["approved_at"]   = src["approved_at"];
            r["released_at"]   = src["released_at"];
            return r;
        };
        ds.create(make_minimal(make_ord_a_json()));  // status=2
        ds.create(make_minimal(make_ord_b_json()));  // status=0
        ds.create(make_minimal(make_ord_c_json()));  // status=4
    }

    OrderRepository repo(path);
    auto producing = repo.find_by_status(2);

    ASSERT_EQ(producing.size(), size_t(1));
    EXPECT_EQ(producing[0].order_number, "ORD-20240501-001");

    remove_file(path);
}

// ── TC-MT-16: OrderRepository::find_by_order_number — 존재 ──────────────────
TEST(TG_MT, TC_MT_16_OrderRepositoryFindByOrderNumberFound) {
    std::string path = make_temp_path("tc_mt_16_orders.json");
    remove_file(path);

    {
        DataStore ds(path);
        JsonValue r = JsonValue::object();
        r["order_number"]  = JsonValue(std::string("ORD-20240501-001"));
        r["sample_id"]     = JsonValue(std::string("SMP-001"));
        r["customer_name"] = JsonValue(std::string("홍길동"));
        r["order_quantity"]= JsonValue(int64_t(50));
        r["order_status"]  = JsonValue(int64_t(2));
        r["approved_at"]   = JsonValue(std::string("2024-05-01 10:30:00"));
        r["released_at"]   = JsonValue(nullptr);
        ds.create(r);
    }

    OrderRepository repo(path);
    auto result = repo.find_by_order_number("ORD-20240501-001");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->approved_at, "2024-05-01 10:30:00");

    remove_file(path);
}

// ── TC-MT-17: OrderRepository::find_by_order_number — 미존재 ─────────────────
TEST(TG_MT, TC_MT_17_OrderRepositoryFindByOrderNumberNotFound) {
    std::string path = make_temp_path("tc_mt_17_orders.json");
    remove_file(path);

    {
        DataStore ds(path);
        JsonValue r = JsonValue::object();
        r["order_number"]  = JsonValue(std::string("ORD-20240501-001"));
        r["sample_id"]     = JsonValue(std::string("SMP-001"));
        r["customer_name"] = JsonValue(std::string("홍길동"));
        r["order_quantity"]= JsonValue(int64_t(50));
        r["order_status"]  = JsonValue(int64_t(2));
        r["approved_at"]   = JsonValue(std::string("2024-05-01 10:30:00"));
        r["released_at"]   = JsonValue(nullptr);
        ds.create(r);
    }

    OrderRepository repo(path);
    auto result = repo.find_by_order_number("ORD-NONEXISTENT");

    EXPECT_FALSE(result.has_value());

    remove_file(path);
}

// ── TC-MT-18: ProductionRepository::find_all ─────────────────────────────────
TEST(TG_MT, TC_MT_18_ProductionRepositoryFindAll) {
    std::string path = make_temp_path("tc_mt_18_productions.json");
    remove_file(path);

    {
        DataStore ds(path);
        JsonValue r = JsonValue::object();
        r["order_number"]         = JsonValue(std::string("ORD-20240501-001"));
        r["sample_name"]          = JsonValue(std::string("산화철 나노입자"));
        r["order_quantity"]       = JsonValue(int64_t(50));
        r["shortage"]             = JsonValue(int64_t(20));
        r["actual_production"]    = JsonValue(int64_t(23));
        r["ordered_at"]           = JsonValue(std::string("2024-05-01 08:00:00"));
        r["estimated_completion"] = JsonValue(std::string("03:30"));
        r["production_start_at"]  = JsonValue(std::string("2024-05-01 10:30:00"));
        ds.create(r);
    }

    ProductionRepository repo(path);
    auto all = repo.find_all();

    ASSERT_EQ(all.size(), size_t(1));
    EXPECT_EQ(all[0].production_start_at, "2024-05-01 10:30:00");

    remove_file(path);
}

// ── TC-MT-19: ProductionRepository::find_by_order_number — 존재 ──────────────
TEST(TG_MT, TC_MT_19_ProductionRepositoryFindByOrderNumberFound) {
    std::string path = make_temp_path("tc_mt_19_productions.json");
    remove_file(path);

    {
        DataStore ds(path);
        JsonValue r = JsonValue::object();
        r["order_number"]         = JsonValue(std::string("ORD-20240501-001"));
        r["sample_name"]          = JsonValue(std::string("산화철 나노입자"));
        r["order_quantity"]       = JsonValue(int64_t(50));
        r["shortage"]             = JsonValue(int64_t(20));
        r["actual_production"]    = JsonValue(int64_t(23));
        r["ordered_at"]           = JsonValue(std::string("2024-05-01 08:00:00"));
        r["estimated_completion"] = JsonValue(std::string("03:30"));
        r["production_start_at"]  = JsonValue(std::string("2024-05-01 10:30:00"));
        ds.create(r);
    }

    ProductionRepository repo(path);
    auto result = repo.find_by_order_number("ORD-20240501-001");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->production_start_at, "2024-05-01 10:30:00");

    remove_file(path);
}

// ── TC-MT-20: ProductionRepository::find_by_order_number — 미존재 ────────────
TEST(TG_MT, TC_MT_20_ProductionRepositoryFindByOrderNumberNotFound) {
    std::string path = make_temp_path("tc_mt_20_productions.json");
    remove_file(path);

    {
        DataStore ds(path);
        JsonValue r = JsonValue::object();
        r["order_number"]         = JsonValue(std::string("ORD-20240501-001"));
        r["sample_name"]          = JsonValue(std::string("산화철 나노입자"));
        r["order_quantity"]       = JsonValue(int64_t(50));
        r["shortage"]             = JsonValue(int64_t(20));
        r["actual_production"]    = JsonValue(int64_t(23));
        r["ordered_at"]           = JsonValue(std::string("2024-05-01 08:00:00"));
        r["estimated_completion"] = JsonValue(std::string("03:30"));
        r["production_start_at"]  = JsonValue(std::string("2024-05-01 10:30:00"));
        ds.create(r);
    }

    ProductionRepository repo(path);
    auto result = repo.find_by_order_number("ORD-NONEXISTENT");

    EXPECT_FALSE(result.has_value());

    remove_file(path);
}
