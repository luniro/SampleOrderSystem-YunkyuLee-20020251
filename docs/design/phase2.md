# Phase 2 상세 설계 — 시료 관리 (메뉴 1)

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

메인 메뉴 1번 "시료 관리"를 구현한다. 하위 메뉴는 등록·조회·검색 세 가지이며, 쓰기 작업은 `DataStore`로, 읽기 작업은 `SampleRepository`로 처리한다.

---

## 2. 파일 구조 변경

```
src/mvc/
├── App.hpp     ← 수정: 멤버 변수 추가, private 메서드 확장
└── App.cpp     ← 수정: 시료 관리 구현
```

다른 파일은 변경하지 않는다.

---

## 3. `App` 클래스 재구조화

### 3.1 멤버 변수

현재 `run()` 내부 지역 변수인 repository를 멤버로 격상하고, 쓰기용 `DataStore`를 추가한다.

```cpp
// App.hpp
#pragma once
#include "data_store.hpp"
#include "monitor/repository/sample_repository.hpp"
#include "monitor/repository/order_repository.hpp"
#include "monitor/repository/production_repository.hpp"
#include <string>

namespace mvc {

struct AppConfig {
    std::string data_dir = ".";
};

class App {
public:
    explicit App(AppConfig config);
    void run();

private:
    AppConfig config_;

    // 쓰기용 DataStore (persistence layer)
    DataStore sample_store_;

    // 읽기용 Repository (monitor layer)
    SampleRepository    sample_repo_;
    OrderRepository     order_repo_;
    ProductionRepository production_repo_;

    // 메인 메뉴 핸들러
    void menu_sample_management();
    void menu_order_reception();
    void menu_order_processing();
    void menu_monitoring();
    void menu_release_processing();
    void menu_production_line();

    // 시료 관리 서브 메서드
    void sample_register();
    void sample_list();
    void sample_search();

    // 출력 헬퍼 (static)
    static void print_sample_detail(const Sample& s);
    static void print_sample_table(const std::vector<Sample>& samples);
};

} // namespace mvc
```

### 3.2 생성자

파일 경로를 constructor initializer list에서 산출하여 초기화한다.

```cpp
namespace fs = std::filesystem;

App::App(AppConfig config)
    : config_(std::move(config))
    , sample_store_((fs::path(config_.data_dir) / "samples.json").string())
    , sample_repo_((fs::path(config_.data_dir) / "samples.json").string())
    , order_repo_((fs::path(config_.data_dir) / "orders.json").string())
    , production_repo_((fs::path(config_.data_dir) / "productions.json").string())
{}
```

> `order_store_`, `production_store_` 는 해당 Phase에서 추가한다. Phase 2에서는 `sample_store_` 만 사용한다.

### 3.3 `run()` 변경

지역 변수로 생성하던 repository 코드를 제거하고 멤버 변수를 사용한다.

---

## 4. 서브 메뉴 루프

### 4.1 화면 구성

```
[ 1. 시료 관리 ]
 1. 시료 등록
 2. 시료 조회
 3. 시료 검색
 0. 돌아가기
> 
```

- `InputUtil::read_int("> ", 0, 3)` 로 입력받는다.
- `0` 입력 시 메인 메뉴로 복귀.

---

## 5. 시료 등록 (`sample_register`)

### 5.1 흐름

```
시료 ID 입력  →  중복 검사  →  시료명 입력  →  평균 생산시간 입력
    →  수율 입력  →  현재 재고 입력  →  DataStore.create()  →  결과 출력
```

### 5.2 입력 항목 및 검증 규칙

| 항목 | 입력 방식 | 검증 조건 | 오류 시 |
|------|----------|-----------|---------|
| 시료 ID (`sample_id`) | `InputUtil::read_nonempty` | 동일 `sample_id` 없을 것 (FR-S-02) | 오류 메시지 출력 후 재입력 |
| 시료명 (`sample_name`) | `InputUtil::read_nonempty` | 동일 `sample_name` 없을 것 (FR-S-02) | 오류 메시지 출력 후 재입력 |
| 평균 생산시간 (`avg_production_time`) | `read_nonempty` → `std::stod` | > 0.0, 동일 값 없을 것 (FR-S-02) | 오류 메시지 출력 후 재입력 |
| 수율 (`yield_rate`) | `read_nonempty` → `std::stod` | [0.70, 0.99], 동일 값 없을 것 (FR-S-02) | 오류 메시지 출력 후 재입력 |
| 현재 재고 (`current_stock`) | `InputUtil::read_int(0, INT_MAX)` | ≥ 0 | (read_int가 처리) |

- 중복 검사 대상: `sample_id`, `sample_name`, `avg_production_time`, `yield_rate` (PRD: "시료 이름과 각 속성값은 시스템 내에서 고유하다")
- `current_stock`은 동적으로 변하는 값이므로 중복 검사 대상에서 제외한다.
- 중복 검사는 `sample_repo_.refresh()` 후 `find_all()` 로 전체 목록을 가져와 순회 비교한다.
- float 파싱 실패(`std::invalid_argument`, `std::out_of_range`) 시 오류 메시지 출력 후 루프 재진입.
- float 중복 비교는 `==` 연산자(exact match)를 사용한다.

### 5.3 저장

```cpp
JsonValue rec = JsonValue::object();
rec["sample_id"]           = JsonValue(sample_id);
rec["sample_name"]         = JsonValue(sample_name);
rec["avg_production_time"] = JsonValue(avg_production_time);
rec["yield_rate"]          = JsonValue(yield_rate);
rec["current_stock"]       = JsonValue(static_cast<int64_t>(current_stock));
sample_store_.create(rec);
sample_repo_.refresh();
```

### 5.4 출력 (FR-S-03)

```
시료가 등록되었습니다.
  시료 ID: S-001
```

---

## 6. 시료 조회 (`sample_list`) — FR-S-04

```cpp
sample_repo_.refresh();
auto samples = sample_repo_.find_all();
if (samples.empty()) {
    std::cout << "등록된 시료가 없습니다.\n";
    return;
}
print_sample_table(samples);  // TablePrinter::print_paged() 사용
```

### 6.1 테이블 컬럼

| 시료 ID | 시료명 | 평균 생산시간(min/ea) | 수율(%) | 현재 재고(ea) |
|---------|--------|----------------------|---------|--------------|

- `avg_production_time`: 소수점 1자리 (`std::fixed`, `std::setprecision(1)`)
- `yield_rate`: `static_cast<int>(yield_rate * 100)` → `"90"`

---

## 7. 시료 검색 (`sample_search`) — FR-S-05~07, FR-S-06b

### 7.1 검색 서브 메뉴

```
검색 방법을 선택하세요:
 1. 시료명 검색 (부분 일치)
 2. 시료 ID 검색 (정확 일치)
 3. 수율 검색 (정확 일치)
 4. 평균 생산시간 검색 (정확 일치)
 0. 돌아가기
> 
```

- `InputUtil::read_int("> ", 0, 4)`

### 7.2 시료명 부분 검색 (FR-S-05, FR-S-06)

```cpp
std::string keyword = InputUtil::read_nonempty("검색어: ");
sample_repo_.refresh();
auto all = sample_repo_.find_all();
std::vector<Sample> found;
for (const auto& s : all)
    if (s.sample_name.find(keyword) != std::string::npos)
        found.push_back(s);
```

| 결과 건수 | 출력 |
|----------|------|
| 0건 | `"검색 결과가 없습니다.\n"` (FR-S-07) |
| 1건 | `print_sample_detail(found[0])` |
| 복수 | `print_sample_table(found)` (FR-S-06) |

### 7.3 속성값 정확 검색 (FR-S-06b)

속성값은 모두 고유하므로 결과는 항상 0건 또는 1건이다.

**시료 ID 검색**

```cpp
std::string sid = InputUtil::read_nonempty("시료 ID: ");
sample_repo_.refresh();
auto result = sample_repo_.find_by_sample_id(sid);
// result 없으면 "검색 결과가 없습니다.", 있으면 print_sample_detail
```

**수율 검색**

```cpp
// read_nonempty → stod, [0.0, 1.0] 파싱 실패 시 재입력
sample_repo_.refresh();
auto all = sample_repo_.find_all();
// s.yield_rate == target_yield 인 첫 번째 항목 → print_sample_detail
// 없으면 "검색 결과가 없습니다."
```

**평균 생산시간 검색**

```cpp
// read_nonempty → stod, > 0.0 파싱 실패 시 재입력
sample_repo_.refresh();
auto all = sample_repo_.find_all();
// s.avg_production_time == target_pt 인 첫 번째 항목 → print_sample_detail
// 없으면 "검색 결과가 없습니다."
```

---

## 8. 출력 헬퍼

### 8.1 `print_sample_detail`

```
[ 시료 상세 ]
  시료 ID       : S-001
  시료명        : 실리콘 웨이퍼 - 8인치
  평균 생산시간 : 1.5 min/ea
  수율          : 90 %
  현재 재고     : 350 ea
```

### 8.2 `print_sample_table`

`TablePrinter` 생성 → 각 행 추가 → `print_paged()` 호출.  
컬럼: `시료 ID`, `시료명`, `평균생산시간(min/ea)`, `수율(%)`, `현재재고(ea)`

---

## 9. 테스트 범위

`tests/mvc/test_sample_management.cpp` — 테스트 그룹 TG-SM

| TC | 시나리오 |
|----|---------|
| TC-SM-01 | 정상 등록 → 조회로 확인 |
| TC-SM-02 | 중복 시료 ID 등록 거부 |
| TC-SM-03 | avg_production_time 비정수/음수 입력 → 오류 후 재입력 |
| TC-SM-04 | yield_rate 범위 초과 → 오류 후 재입력 |
| TC-SM-05 | current_stock 음수 → 오류 후 재입력 |
| TC-SM-06 | 시료 조회 — 빈 DB에서 안내 메시지 |
| TC-SM-07 | 시료 조회 — 여러 시료 테이블 출력 확인 |
| TC-SM-08 | 시료명 부분 검색 — 0건 |
| TC-SM-09 | 시료명 부분 검색 — 1건 → 상세 출력 |
| TC-SM-10 | 시료명 부분 검색 — 복수건 → 테이블 출력 |
| TC-SM-11 | 시료 ID 검색 — 존재 → 상세 출력 |
| TC-SM-12 | 시료 ID 검색 — 미존재 → 안내 메시지 |
| TC-SM-13 | 등록 완료 후 시료 ID 출력 확인 (FR-S-03) |
| TC-SM-14 | 중복 시료명 등록 거부 |
| TC-SM-15 | 중복 avg_production_time 등록 거부 |
| TC-SM-16 | 중복 yield_rate 등록 거부 |
| TC-SM-17 | 수율 정확 검색 — 존재 → 상세 출력 |
| TC-SM-18 | 수율 정확 검색 — 미존재 → 안내 메시지 |
| TC-SM-19 | 평균 생산시간 정확 검색 — 존재 → 상세 출력 |
| TC-SM-20 | 평균 생산시간 정확 검색 — 미존재 → 안내 메시지 |
