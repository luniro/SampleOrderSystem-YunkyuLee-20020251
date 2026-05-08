# monitor

`DataStore`(JSON 파일 기반 CRUD) 위에서 동작하는 콘솔 조회 어댑터 모듈.  
`src/monitor/` 폴더 단위로 다른 프로젝트에 이식 가능하도록 설계됐다.

---

## 의존성

이 모듈은 아래 두 라이브러리가 대상 프로젝트에 이미 존재한다고 가정한다.

| 라이브러리 | 역할 | CMake 타겟명 |
|-----------|------|-------------|
| `persistence` | `DataStore` CRUD (`data_store.hpp`) | `persistence` |
| `json_lib` | JSON 파싱·직렬화 (`json/json.hpp`) | `json_lib` |

`persistence`가 `json_lib`에 PUBLIC으로 링크되어 있으면 `monitor_lib`은 `persistence`만 링크해도 된다.

---

## CMake 통합

### 1. 서브디렉터리 추가

```cmake
add_subdirectory(path/to/monitor)
```

### 2. 실행 파일에 링크

```cmake
target_link_libraries(YourApp PRIVATE monitor_lib)
```

`monitor_lib`의 PUBLIC 포함 경로(`${CMAKE_CURRENT_SOURCE_DIR}`)가 자동으로 전파되므로
`target_include_directories`를 별도로 추가할 필요 없다.

---

## 도메인 타입 (`domain/types.hpp`)

세 가지 도메인 구조체를 정의하며, 각각 `DataStore`의 `JsonValue`로부터 변환하는
`static from_json(const JsonValue&)` 메서드를 제공한다.

### Sample

| 필드 | 타입 | 설명 |
|------|------|------|
| `id` | `int64_t` | DataStore 내부 키 |
| `sample_id` | `std::string` | 시료 고유 ID (예: `S-001`) |
| `sample_name` | `std::string` | 시료명 |
| `avg_production_time` | `double` | 평균 생산시간 (min/ea) |
| `yield_rate` | `double` | 수율 (0.0 ~ 1.0) |
| `current_stock` | `int64_t` | 현재 재고 (개) |

### Order

| 필드 | 타입 | 설명 |
|------|------|------|
| `id` | `int64_t` | DataStore 내부 키 |
| `order_number` | `std::string` | 주문번호 (예: `ORD-20240501-001`) |
| `sample_id` | `std::string` | 참조 시료 ID |
| `customer_name` | `std::string` | 고객명 |
| `order_quantity` | `int64_t` | 주문수량 (개) |
| `order_status` | `int` | 주문상태 (아래 상수 참조) |

**주문상태 상수**

```cpp
Order::STATUS_RESERVED   // 0
Order::STATUS_REJECTED   // 1
Order::STATUS_PRODUCING  // 2
Order::STATUS_CONFIRMED  // 3
Order::STATUS_RELEASE    // 4
```

### Production

| 필드 | 타입 | 설명 |
|------|------|------|
| `id` | `int64_t` | DataStore 내부 키 |
| `order_number` | `std::string` | 참조 주문번호 |
| `sample_name` | `std::string` | 시료명 |
| `order_quantity` | `int64_t` | 주문수량 (개) |
| `shortage` | `int64_t` | 부족분 (개) |
| `actual_production` | `int64_t` | 실제 생산량 (개) |
| `ordered_at` | `std::string` | 주문 접수 시각 (`HH:MM`) |
| `estimated_completion` | `std::string` | 총 생산 소요시간 (`HH:MM`) |

---

## Repository API

각 Repository는 `DataStore`를 래핑하여 타입 안전한 조회 인터페이스를 제공한다.  
생성자에 JSON 파일 경로를 전달한다. `refresh()`를 호출하면 파일을 다시 읽어 변경 내용을 반영한다.

```cpp
// SampleRepository
SampleRepository repo("samples.json");
std::vector<Sample>   all     = repo.find_all();
std::optional<Sample> one     = repo.find_by_sample_id("S-001");
repo.refresh();

// OrderRepository
OrderRepository repo("orders.json");
std::vector<Order>   all      = repo.find_all();
std::vector<Order>   filtered = repo.find_by_status(Order::STATUS_PRODUCING);
std::optional<Order> one      = repo.find_by_order_number("ORD-20240501-001");
repo.refresh();

// ProductionRepository
ProductionRepository repo("productions.json");
std::vector<Production>   all = repo.find_all();
std::optional<Production> one = repo.find_by_order_number("ORD-20240501-001");
repo.refresh();
```

---

## 콘솔 App

`App`은 숫자 메뉴 루프를 제공한다. `DataPaths`로 각 JSON 파일 경로를 지정한다.

```cpp
#include "ui/app.hpp"

App app({
    .samples     = "path/to/samples.json",
    .orders      = "path/to/orders.json",
    .productions = "path/to/productions.json"
});
app.run();
```

메뉴 항목:

```
1. 시료(Sample) 목록
2. 주문(Order) 목록
3. 주문(Order) 상태별 조회
4. 생산(Production) 현황
0. 종료
```

App 없이 Repository와 TablePrinter만 단독으로 사용할 수도 있다.

```cpp
#include "repository/sample_repository.hpp"
#include "ui/table_printer.hpp"

SampleRepository repo("samples.json");
TablePrinter tp({"ID", "sample_id", "sample_name"});
for (const auto& s : repo.find_all())
    tp.add_row({std::to_string(s.id), s.sample_id, s.sample_name});
tp.print();
```

---

## 파일 구조

```
monitor/
├── CMakeLists.txt
├── README.md
├── domain/
│   ├── types.hpp          # Sample / Order / Production 구조체
│   └── types.cpp          # from_json 구현
├── repository/
│   ├── sample_repository.hpp / .cpp
│   ├── order_repository.hpp / .cpp
│   └── production_repository.hpp / .cpp
└── ui/
    ├── table_printer.hpp / .cpp   # 컬럼 너비 자동 조정 출력
    └── app.hpp / .cpp             # 메뉴 루프
```

---

## 주의 사항

- 이 모듈은 **조회 전용**이다. `create` / `update` / `remove`는 노출하지 않는다.
- `DataStore`는 파일 잠금을 하지 않으므로, 다른 프로세스가 동시에 쓰는 경우 불일치가 발생할 수 있다.
- 파일이 존재하지 않으면 `DataStore`가 빈 컬렉션으로 초기화되어 조회 결과가 비어 있게 된다.
