# Phase 4 상세 설계 — 주문 처리 (메뉴 3)

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

메인 메뉴 3번 "주문 처리"를 구현한다. Reserved 주문 목록에서 선택 → 재고 판정(PRD 4.3) → 승인/거절/취소 순으로 진행되며, 이 단계에 핵심 비즈니스 로직이 집중된다.

---

## 2. 파일 구조 변경

```
src/mvc/
├── App.hpp     ← 수정: production_store_ 멤버 추가, order_process 메서드 추가
└── App.cpp     ← 수정: menu_order_processing() 구현
```

---

## 3. `App` 클래스 변경

### 3.1 멤버 추가

```cpp
// private
DataStore production_store_;   // 신규: 생산 레코드 쓰기용
```

### 3.2 생성자 initializer list 추가

```cpp
, production_store_((fs::path(config_.data_dir) / "productions.json").string())
```

(기존 `sample_store_`, `order_store_`, 각 repo 뒤에 추가)

### 3.3 메서드 추가

```cpp
// App.hpp private
void order_process_list();                      // Reserved 목록 표시 + 선택
void order_process_detail(const Order& order);  // 재고 확인 + 승인/거절/취소
void order_approve(const Order& order);         // 승인 처리
void order_reject(const Order& order);          // 거절 처리
```

---

## 4. 처리 흐름

```
menu_order_processing()
  └─ order_process_list()
       │
       ├─ Reserved 주문 없음 → "처리 대기 중인 주문이 없습니다." → 반환
       │
       ├─ 목록 출력 (TablePrinter)
       │   컬럼: No, 주문번호, 고객, 시료ID, 수량(ea)
       │
       ├─ 번호 입력 (0: 돌아가기)
       │   잘못된 번호 → 재입력
       │
       └─ order_process_detail(selected_order)
            │
            ├─ 재고 확인 화면 출력 (FR-A-03~04)
            │
            ├─ 승인(1) / 거절(2) / 취소(0) 선택 (FR-A-05~06)
            │
            ├─ 취소(0) → order_process_list()로 복귀
            │
            ├─ 승인(1) → order_approve(order)
            │
            └─ 거절(2) → order_reject(order)
```

`menu_order_processing()`은 `order_process_list()`를 루프로 반복 호출한다 (처리 후 목록 재표시).

---

## 5. Reserved 주문 목록 (FR-A-01~02)

```cpp
order_repo_.refresh();
auto reserved = order_repo_.find_by_status(Order::STATUS_RESERVED);
```

빈 경우: `"처리 대기 중인 주문이 없습니다.\n"` 출력 후 반환.

테이블 컬럼: `No`, `주문번호`, `고객`, `시료ID`, `수량(ea)`  
`TablePrinter::print()` 사용 (paged 불필요, 목록이 짧을 것으로 가정).

번호 입력: `InputUtil::read_int("번호를 입력하세요 (0: 돌아가기): ", 0, reserved.size())`

---

## 6. 재고 확인 및 판정 (FR-A-03~04)

### 6.1 가용 재고 산출 (PRD 4.3)

```
가용 재고 = current_stock
          − Σ(order_quantity | 동일 sample_id, 상태 = Confirmed)
```

```cpp
sample_repo_.refresh();
auto sample_opt = sample_repo_.find_by_sample_id(order.sample_id);
int64_t current_stock = sample_opt->current_stock;

order_repo_.refresh();
int64_t confirmed_sum = 0;
for (const auto& o : order_repo_.find_all()) {
    if (o.sample_id == order.sample_id && o.order_status == Order::STATUS_CONFIRMED)
        confirmed_sum += o.order_quantity;
}
int64_t available = std::max(int64_t(0), current_stock - confirmed_sum);
```

### 6.2 부족분 계산 (PRD 4.3 Case 1/2)

```cpp
bool has_producing = false;
for (const auto& o : order_repo_.find_all()) {
    if (o.sample_id == order.sample_id && o.order_status == Order::STATUS_PRODUCING)
        has_producing = true;
}

bool needs_production = (available < order.order_quantity);
int64_t shortage = 0;
if (needs_production) {
    if (!has_producing) {
        shortage = order.order_quantity - available;  // Case 1
    } else {
        shortage = order.order_quantity;              // Case 2
    }
}
```

### 6.3 실 생산량·소요 시간 산출

```cpp
int64_t actual_prod = ProductionCalc::actual_production(shortage, sample_opt->yield_rate);
double  est_minutes = ProductionCalc::estimated_minutes(actual_prod, sample_opt->avg_production_time);
std::string est_duration = ProductionCalc::format_duration(est_minutes);
```

### 6.4 화면 출력 (FR-A-03~04)

```
[ 재고 확인 ]
  시료    : 실리콘 웨이퍼 - 8인치
  주문수량: 100 ea
  현재재고: 50 ea  (가용 재고, 선점분 제외)
  부족분  : 50 ea
```

부족분 > 0 이면 추가 출력 (FR-A-04):
```
  실 생산량: 66 ea
  생산 소요: 00:33
```

재고 충분(shortage = 0)이면 부족분 행만 표시 (`0 ea`), 실 생산량·생산 소요 미출력.

---

## 7. 승인/거절/취소 (FR-A-05~06)

```
 1. 승인  2. 거절  0. 취소
> 
```

`InputUtil::read_int("> ", 0, 2)`

- `0`: 목록으로 복귀 (FR-A-06)
- `1`: `order_approve(order)` 호출
- `2`: `order_reject(order)` 호출

---

## 8. 승인 처리 — `order_approve` (FR-A-07)

### 8.1 `approved_at` 기록

```cpp
std::string approved_at = Timestamp::now();  // "YYYY-MM-DD HH:MM:SS" UTC
```

### 8.2 재고 충분 → Confirmed 전환

```cpp
JsonValue upd = JsonValue::object();
upd["order_status"] = JsonValue(int64_t(Order::STATUS_CONFIRMED));
upd["approved_at"]  = JsonValue(approved_at);
order_store_.update(order.id, upd);
order_repo_.refresh();
```

### 8.3 재고 부족 → Producing 전환 + 생산 레코드 생성

**주문 상태 업데이트:**
```cpp
JsonValue upd = JsonValue::object();
upd["order_status"] = JsonValue(int64_t(Order::STATUS_PRODUCING));
upd["approved_at"]  = JsonValue(approved_at);
order_store_.update(order.id, upd);
order_repo_.refresh();
```

**`production_start_at` 확정 (enqueue 규칙):**

현재 Producing 상태인 모든 주문의 생산 레코드를 조회하여 큐 마지막 완료 시각을 구한다.

```cpp
production_repo_.refresh();

// 현재 Producing 주문번호 집합
std::set<std::string> producing_nums;
for (const auto& o : order_repo_.find_all())
    if (o.order_status == Order::STATUS_PRODUCING && o.order_number != order.order_number)
        producing_nums.insert(o.order_number);

// 큐 마지막 완료 시각 = max(production_start_at + estimated_completion)
int64_t max_end_epoch = 0;
for (const auto& p : production_repo_.find_all()) {
    if (producing_nums.count(p.order_number) == 0) continue;
    int64_t end_epoch = Timestamp::completion_epoch(p.production_start_at, p.estimated_completion);
    if (end_epoch > max_end_epoch) max_end_epoch = end_epoch;
}

std::string production_start_at;
if (max_end_epoch == 0) {
    production_start_at = approved_at;        // 큐 비어있음: approved_at 사용
} else {
    production_start_at = Timestamp::format(max_end_epoch);  // 선행 완료 시각
}
```

**생산 레코드 저장:**
```cpp
JsonValue prod = JsonValue::object();
prod["order_number"]         = JsonValue(order.order_number);
prod["sample_name"]          = JsonValue(sample_opt->sample_name);
prod["order_quantity"]       = JsonValue(order.order_quantity);
prod["shortage"]             = JsonValue(shortage);
prod["actual_production"]    = JsonValue(actual_prod);
prod["ordered_at"]           = JsonValue(approved_at);
prod["estimated_completion"] = JsonValue(est_duration);
prod["production_start_at"]  = JsonValue(production_start_at);
production_store_.create(prod);
production_repo_.refresh();
```

### 8.4 결과 출력 (FR-A-09)

```
처리 완료.
  주문번호: ORD-20240501-001
  상태    : Confirmed  (또는 Producing)
```

---

## 9. 거절 처리 — `order_reject` (FR-A-08~09)

```cpp
JsonValue upd = JsonValue::object();
upd["order_status"] = JsonValue(int64_t(Order::STATUS_REJECTED));
order_store_.update(order.id, upd);
order_repo_.refresh();

std::cout << "처리 완료.\n"
          << "  주문번호: " << order.order_number << "\n"
          << "  상태    : Rejected\n";
```

---

## 10. 테스트 범위

`tests/mvc/test_order_processing.cpp` — 테스트 그룹 TG-AP

| TC | 시나리오 |
|----|---------|
| TC-AP-01 | Reserved 주문 없을 때 안내 메시지 출력 |
| TC-AP-02 | 재고 충분(shortage=0) → 승인 → Confirmed 전환 확인 |
| TC-AP-03 | 재고 부족, Producing 없음(Case 1) → 승인 → Producing 전환 + 생산 레코드 생성 확인 |
| TC-AP-04 | 재고 부족, Producing 이미 존재(Case 2) → shortage = order_quantity 확인 |
| TC-AP-05 | 거절 → Rejected 전환 확인 |
| TC-AP-06 | 취소(0) → 주문 상태 미변경 확인 |
| TC-AP-07 | approved_at 기록 확인 (승인 시) |
| TC-AP-08 | production_start_at — 큐 빈 경우 = approved_at |
| TC-AP-09 | production_start_at — 선행 Producing 있을 경우 = 선행 완료 시각 |
| TC-AP-10 | 가용 재고 계산: Confirmed 선점분 차감 확인 |
