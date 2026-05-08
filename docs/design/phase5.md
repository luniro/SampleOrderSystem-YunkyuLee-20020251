# Phase 5 상세 설계 — 모니터링 (메뉴 4)

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

메인 메뉴 4번 "모니터링"을 구현한다. 현재 `menu_monitoring()`은 `"준비 중"` stub이다. 이를 두 개의 서브 기능으로 교체한다.

- **주문량 확인** (FR-M-01~02): 주문 상태별 건수를 표시한다. Rejected는 제외.
- **재고량 확인** (FR-M-03~05): 시료별 현재 재고와 재고 상태(고갈/여유/부족)를 테이블로 표시한다.

두 기능 모두 화면 진입 시마다 파일에서 최신 데이터를 읽는다 (FR-M-05).

---

## 2. 파일 구조 변경

```
src/mvc/
├── App.hpp   ← 수정: monitoring 서브 메서드 선언 추가
└── App.cpp   ← 수정: menu_monitoring() 구현 + 서브 메서드 구현
```

신규 파일 없음. 기존 App 클래스에 메서드만 추가한다.

---

## 3. `App` 클래스 변경

### 3.1 App.hpp — private 메서드 추가

```cpp
// 모니터링 서브 메서드
void monitoring_order_count();   // FR-M-01~02
void monitoring_stock_level();   // FR-M-03~05
```

기존 멤버(`sample_repo_`, `order_repo_`, `production_repo_`)를 그대로 사용한다. 신규 멤버 없음.

---

## 4. 처리 흐름

```
menu_monitoring()
  │
  └─ 서브 메뉴 루프
       │
       ├─ 1. 주문량 확인 → monitoring_order_count()
       │
       ├─ 2. 재고량 확인 → monitoring_stock_level()
       │
       └─ 0. 돌아가기
```

### 서브 메뉴 출력 형식

```
[ 4. 모니터링 ]
 1. 주문량 확인
 2. 재고량 확인
 0. 돌아가기
> 
```

`InputUtil::read_int("> ", 0, 2)` 사용.

---

## 5. 주문량 확인 — `monitoring_order_count` (FR-M-01~02)

### 5.1 데이터 로드

```cpp
order_repo_.refresh();
auto all_orders = order_repo_.find_all();
```

### 5.2 집계

```cpp
int cnt_reserved  = 0;
int cnt_producing = 0;
int cnt_confirmed = 0;
int cnt_released  = 0;

for (const auto& o : all_orders) {
    switch (o.order_status) {
        case Order::STATUS_RESERVED:  ++cnt_reserved;  break;
        case Order::STATUS_PRODUCING: ++cnt_producing; break;
        case Order::STATUS_CONFIRMED: ++cnt_confirmed; break;
        case Order::STATUS_RELEASED:  ++cnt_released;  break;
        default: break;  // STATUS_REJECTED 제외
    }
}
```

### 5.3 화면 출력

```
[ 주문량 현황 ]
  Reserved  : N 건
  Producing : N 건
  Confirmed : N 건
  Released  : N 건
```

건수가 0인 상태도 반드시 표시한다 (FR-M-02).

---

## 6. 재고량 확인 — `monitoring_stock_level` (FR-M-03~05)

### 6.1 데이터 로드

```cpp
sample_repo_.refresh();
order_repo_.refresh();
auto samples    = sample_repo_.find_all();
auto all_orders = order_repo_.find_all();
```

### 6.2 재고 상태 판정 로직

각 시료에 대해 판정 우선순위: **고갈 → 여유 → 부족** (PRD 6.4 FR-M-04)

```cpp
auto stock_status = [&](const Sample& s) -> std::string {
    // 1. 고갈
    if (s.current_stock == 0) return "고갈";

    // 2. Producing 존재 여부 확인
    bool has_producing = false;
    int64_t reserved_sum  = 0;
    int64_t confirmed_sum = 0;
    for (const auto& o : all_orders) {
        if (o.sample_id != s.sample_id) continue;
        if (o.order_status == Order::STATUS_PRODUCING) has_producing = true;
        if (o.order_status == Order::STATUS_RESERVED)  reserved_sum  += o.order_quantity;
        if (o.order_status == Order::STATUS_CONFIRMED) confirmed_sum += o.order_quantity;
    }

    // 3. 여유: Producing 없음 AND current_stock >= Reserved합 + Confirmed합
    if (!has_producing && s.current_stock >= reserved_sum + confirmed_sum)
        return "여유";

    // 4. 부족: 나머지
    return "부족";
};
```

### 6.3 테이블 출력

컬럼: `시료명`, `현재재고(ea)`, `재고 상태`

```cpp
if (samples.empty()) {
    std::cout << "등록된 시료가 없습니다.\n";
    return;
}

TablePrinter tp({"시료명", "현재재고(ea)", "재고 상태"});
for (const auto& s : samples) {
    tp.add_row({
        s.sample_name,
        std::to_string(s.current_stock),
        stock_status(s)
    });
}
tp.print_paged();
```

시료가 많을 경우를 대비해 `print_paged()` 사용.

---

## 7. 화면 진입 시 최신 데이터 보장 (FR-M-05)

각 서브 메서드(`monitoring_order_count`, `monitoring_stock_level`) 진입 즉시 `refresh()`를 호출한다. `menu_monitoring()` 레벨에서 공통 refresh를 하지 않고, 각 메서드에서 독립적으로 호출한다.

---

## 8. 전체 구현 스니펫

### menu_monitoring()

```cpp
void App::menu_monitoring() {
    bool loop = true;
    while (loop) {
        std::cout << "\n[ 4. 모니터링 ]\n"
                  << " 1. 주문량 확인\n"
                  << " 2. 재고량 확인\n"
                  << " 0. 돌아가기\n";
        int choice = InputUtil::read_int("> ", 0, 2);
        switch (choice) {
            case 1: monitoring_order_count(); break;
            case 2: monitoring_stock_level(); break;
            case 0: loop = false;             break;
            default: break;
        }
    }
}
```

### monitoring_order_count()

```cpp
void App::monitoring_order_count() {
    order_repo_.refresh();
    auto all_orders = order_repo_.find_all();

    int cnt_reserved = 0, cnt_producing = 0, cnt_confirmed = 0, cnt_released = 0;
    for (const auto& o : all_orders) {
        switch (o.order_status) {
            case Order::STATUS_RESERVED:  ++cnt_reserved;  break;
            case Order::STATUS_PRODUCING: ++cnt_producing; break;
            case Order::STATUS_CONFIRMED: ++cnt_confirmed; break;
            case Order::STATUS_RELEASED:  ++cnt_released;  break;
            default: break;
        }
    }

    std::cout << "\n[ 주문량 현황 ]\n"
              << "  Reserved  : " << cnt_reserved  << " 건\n"
              << "  Producing : " << cnt_producing << " 건\n"
              << "  Confirmed : " << cnt_confirmed << " 건\n"
              << "  Released  : " << cnt_released  << " 건\n";
}
```

### monitoring_stock_level()

```cpp
void App::monitoring_stock_level() {
    sample_repo_.refresh();
    order_repo_.refresh();
    auto samples    = sample_repo_.find_all();
    auto all_orders = order_repo_.find_all();

    if (samples.empty()) {
        std::cout << "등록된 시료가 없습니다.\n";
        return;
    }

    TablePrinter tp({"시료명", "현재재고(ea)", "재고 상태"});
    for (const auto& s : samples) {
        bool has_producing = false;
        int64_t reserved_sum = 0, confirmed_sum = 0;
        for (const auto& o : all_orders) {
            if (o.sample_id != s.sample_id) continue;
            if (o.order_status == Order::STATUS_PRODUCING) has_producing = true;
            if (o.order_status == Order::STATUS_RESERVED)  reserved_sum  += o.order_quantity;
            if (o.order_status == Order::STATUS_CONFIRMED) confirmed_sum += o.order_quantity;
        }

        std::string status;
        if (s.current_stock == 0) {
            status = "고갈";
        } else if (!has_producing && s.current_stock >= reserved_sum + confirmed_sum) {
            status = "여유";
        } else {
            status = "부족";
        }

        tp.add_row({s.sample_name, std::to_string(s.current_stock), status});
    }
    tp.print_paged();
}
```

---

## 9. 테스트 범위

`tests/mvc/test_mvc_app.cpp` — 테스트 그룹 TG-MN

| TC | 시나리오 |
|----|---------|
| TC-MN-01 | 주문 없을 때 상태별 건수 모두 0 출력 |
| TC-MN-02 | Reserved 2건, Producing 1건, Confirmed 1건, Released 1건, Rejected 1건 → Rejected 제외하고 각 건수 정확히 출력 |
| TC-MN-03 | 시료 없을 때 "등록된 시료가 없습니다." 출력 |
| TC-MN-04 | current_stock=0 → 재고 상태 "고갈" |
| TC-MN-05 | current_stock>0, Producing 없음, stock >= Reserved합+Confirmed합 → "여유" |
| TC-MN-06 | current_stock>0, Producing 있음 → "부족" |
| TC-MN-07 | current_stock>0, Producing 없음, stock < Reserved합+Confirmed합 → "부족" |
| TC-MN-08 | current_stock=0이면서 Producing도 있는 경우 → 우선순위 "고갈" |
| TC-MN-09 | 여러 시료 혼재 시 시료별 독립 판정 확인 |
| TC-MN-10 | menu_monitoring() 진입마다 파일 최신 데이터 반영(refresh 호출 여부) 확인 |
