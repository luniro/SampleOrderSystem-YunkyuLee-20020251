# Phase 8 상세 설계 — 백그라운드 자동 전환

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

Producing 주문의 완료 예정 시각(`production_start_at + estimated_completion`)이 경과하면 자동으로 Confirmed 전환 및 재고 증가를 수행한다.

| FR | 요약 | 발동 시점 |
|----|------|-----------|
| FR-L-09 | 런타임 자동 전환 | 메인 메뉴 루프 매 반복 시 |
| FR-L-10 | 기동 시 lazy evaluation | 프로그램 첫 루프 진입 시 |

두 요구사항은 **동일한 메서드 한 번 호출**로 처리된다. 메인 메뉴 루프 상단에서 매번 호출하면 첫 번째 호출이 FR-L-10 역할, 이후 호출이 FR-L-09 역할을 자연스럽게 수행한다.

> NFR-03(단일 프로세스)에 따라 실제 백그라운드 스레드는 사용하지 않는다. 사용자가 서브메뉴 안에 머무는 동안에는 전환이 지연되며, 이는 요구사항 허용 범위다.

---

## 2. 파일 구조 변경

```
src/mvc/
├── App.hpp   ← 수정: evaluate_producing_orders() 선언 추가
└── App.cpp   ← 수정: evaluate_producing_orders() 구현 + run() 루프 상단에 호출 추가
              ← 수정: #include <algorithm> 추가
```

신규 파일 없음. 기존 멤버(`order_store_`, `sample_store_`, `order_repo_`, `production_repo_`, `sample_repo_`)를 그대로 사용한다.

---

## 3. `App` 클래스 변경

### 3.1 App.hpp — private 메서드 추가

```cpp
void evaluate_producing_orders();  // FR-L-09 / FR-L-10
```

### 3.2 App.cpp — run() 수정

루프 상단(메뉴 출력 직전)에 평가 호출을 삽입한다.

```cpp
void App::run() {
    bool running = true;
    while (running) {
        evaluate_producing_orders();  // ← 추가 (FR-L-09/FR-L-10)

        std::cout << "\n=== SampleOrderSystem ===\n"
                  << " 1. 시료 관리\n"
                  // ...
    }
}
```

---

## 4. 알고리즘 — `evaluate_producing_orders`

### 4.1 처리 흐름

```
evaluate_producing_orders()
  │
  ├─ order_repo_.refresh() + production_repo_.refresh()
  │
  ├─ Producing 주문 + Production 레코드 쌍 수집
  │   Production 없는 주문 → 스킵 (데이터 불일치)
  │
  ├─ production_start_at 오름차순 정렬 (FR-L-10)
  │
  └─ 각 항목 순차 평가
       │
       ├─ completion_epoch > now_epoch → 스킵 (미완료)
       │
       └─ completion_epoch ≤ now_epoch → 전환 처리
            ├─ order_status → Confirmed
            ├─ sample_repo_.refresh()  (루프 내 최신 재고 보장)
            ├─ current_stock += actual_production
            └─ 전환 알림 출력
```

### 4.2 정렬 기준 (FR-L-10)

`production_start_at`(문자열 `"YYYY-MM-DD HH:MM:SS"`)의 사전순 정렬 = 시간순 정렬이 보장되므로 `std::sort`와 기본 `<` 연산자를 사용한다.

### 4.3 루프 내 `sample_repo_.refresh()` 위치

동일 시료에 대해 여러 Producing 주문이 동시에 완료되는 경우, 두 번째 주문의 재고 증가가 첫 번째 주문의 증가분을 덮어쓰지 않으려면 각 전환 직전에 파일에서 최신 재고를 다시 읽어야 한다.

```
주문 A (시료 X, actual_production=20): stock = 100 → 120 → 파일 저장
주문 B (시료 X, actual_production=15): refresh → stock = 120 → 135 → 파일 저장
```

`sample_repo_.refresh()`를 루프 바깥에서 한 번만 호출하면 주문 B가 `stock=100`을 읽어 `115`로 저장하는 오류가 발생한다.

---

## 5. 전체 구현 스니펫

### App.cpp — `#include` 추가

```cpp
#include <algorithm>   // std::sort
```

### evaluate_producing_orders()

```cpp
void App::evaluate_producing_orders() {
    order_repo_.refresh();
    production_repo_.refresh();

    int64_t now_epoch = Timestamp::parse(Timestamp::now());

    // Producing 주문과 Production 레코드 쌍 수집
    struct QueueEntry { Order order; Production prod; };
    std::vector<QueueEntry> queue;

    for (const auto& o : order_repo_.find_all()) {
        if (o.order_status != Order::STATUS_PRODUCING) continue;
        auto p_opt = production_repo_.find_by_order_number(o.order_number);
        if (!p_opt.has_value()) continue;
        queue.push_back({o, p_opt.value()});
    }

    // production_start_at 오름차순 정렬 (FR-L-10)
    std::sort(queue.begin(), queue.end(),
        [](const QueueEntry& a, const QueueEntry& b) {
            return a.prod.production_start_at < b.prod.production_start_at;
        });

    for (const auto& entry : queue) {
        int64_t comp_epoch = Timestamp::completion_epoch(
            entry.prod.production_start_at, entry.prod.estimated_completion);

        if (comp_epoch > now_epoch) continue;

        // Producing → Confirmed 전환
        JsonValue upd_order = JsonValue::object();
        upd_order["order_status"] = JsonValue(int64_t(Order::STATUS_CONFIRMED));
        order_store_.update(entry.order.id, upd_order);

        // current_stock += actual_production (최신 재고를 파일에서 재조회)
        sample_repo_.refresh();
        auto sample_opt = sample_repo_.find_by_sample_id(entry.order.sample_id);
        if (sample_opt.has_value()) {
            const Sample& s = sample_opt.value();
            JsonValue upd_sample = JsonValue::object();
            upd_sample["current_stock"] = JsonValue(
                s.current_stock + entry.prod.actual_production);
            sample_store_.update(s.id, upd_sample);
        }

        std::cout << "[자동 전환] " << entry.order.order_number
                  << ": Producing → Confirmed"
                  << " (재고 +" << entry.prod.actual_production << " ea)\n";
    }

    order_repo_.refresh();
    sample_repo_.refresh();
}
```

마지막 `refresh()` 두 줄: 전환이 0건이더라도 비용이 미미하므로 조건 없이 호출한다.

---

## 6. 예외 처리

| 상황 | 처리 |
|------|------|
| Producing 주문에 매핑된 Production 레코드 없음 | 해당 주문 스킵 (데이터 불일치, 정상 운영 시 미발생) |
| Production 레코드에 매핑된 Sample 없음 | 주문 상태는 Confirmed로 전환하되 재고 증가 생략 후 계속 |
| `estimated_completion` 파싱 실패 (`completion_epoch` = 0) | `0 ≤ now_epoch` 조건 충족 → 즉시 전환. 데이터 오염 케이스이므로 허용 |

---

## 7. 전환 알림

전환 발생 시 메뉴 출력 직전에 표시된다.

```
[자동 전환] ORD-20240501-002: Producing → Confirmed (재고 +200 ea)
[자동 전환] ORD-20240501-003: Producing → Confirmed (재고 +150 ea)

=== SampleOrderSystem ===
 1. 시료 관리
 ...
```

전환이 없는 경우 아무것도 출력하지 않는다.

---

## 8. 테스트 범위

`tests/mvc/test_background_eval.cpp` — 테스트 그룹 TG-BG

| TC | 시나리오 |
|----|---------|
| TC-BG-01 | Producing 주문 없음 → order·sample 변경 없음 |
| TC-BG-02 | completion_epoch > now → 전환 없음 |
| TC-BG-03 | completion_epoch ≤ now → order_status = Confirmed 확인 |
| TC-BG-04 | 전환 시 current_stock += actual_production 확인 |
| TC-BG-05 | 동일 시료의 Producing 2건 모두 완료 → stock이 두 actual_production 합산분만큼 증가 확인 (중복 덮어쓰기 없음) |
| TC-BG-06 | Producing 2건 중 1건만 완료 → 미완료 주문 상태 불변 확인 |
| TC-BG-07 | 정렬: production_start_at 이른 순으로 처리 확인 (FR-L-10) |
| TC-BG-08 | Production 레코드 없는 Producing 주문 → 스킵, 나머지 정상 처리 |
| TC-BG-09 | run() 첫 루프 진입 시 evaluate 호출 확인 (FR-L-10 기동 평가) |
| TC-BG-10 | 전환 발생 시 "[자동 전환]" 알림 출력 포함 확인 |
