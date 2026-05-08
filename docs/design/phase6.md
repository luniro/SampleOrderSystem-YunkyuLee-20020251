# Phase 6 상세 설계 — 출고 처리 (메뉴 5)

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

메인 메뉴 5번 "출고 처리"를 구현한다. 현재 `menu_release_processing()`은 `"준비 중"` stub이다. Confirmed 주문 목록에서 선택 → 출고/취소 선택 → Released 전환 + 재고 차감 순으로 동작한다.

이 단계에서 `current_stock` 차감이 발생하므로 `sample_store_`에 대한 쓰기 작업이 필요하다. `sample_store_`는 이미 App에 초기화되어 있다.

---

## 2. 파일 구조 변경

```
src/mvc/
├── App.hpp   ← 수정: release 서브 메서드 선언 추가
└── App.cpp   ← 수정: menu_release_processing() 구현 + 서브 메서드 구현
```

신규 파일 없음. 기존 App 클래스에 메서드만 추가한다.

---

## 3. `App` 클래스 변경

### 3.1 App.hpp — private 메서드 추가

```cpp
// 출고 처리 서브 메서드
bool release_process_list();                      // Confirmed 목록 표시 + 선택
void release_process_detail(const Order& order);  // 출고/취소 선택 + 처리
```

기존 멤버(`sample_store_`, `order_store_`, `sample_repo_`, `order_repo_`)를 그대로 사용한다. 신규 멤버 없음.

---

## 4. 처리 흐름

```
menu_release_processing()
  └─ release_process_list() 루프 (Phase 4 menu_order_processing과 동일 패턴)
       │
       ├─ Confirmed 주문 없음 → "출고 가능한 주문이 없습니다." → false 반환
       │
       ├─ 목록 출력 (TablePrinter)
       │   컬럼: No, 주문번호, 고객, 시료ID, 수량(ea)
       │
       ├─ 번호 입력 (0: 돌아가기)
       │   0 → false 반환
       │
       └─ release_process_detail(selected_order)
            │
            ├─ 주문 정보 출력
            │
            ├─ 출고(1) / 취소(0) 선택 (FR-R-03~04)
            │
            ├─ 취소(0) → 변경 없이 목록 복귀
            │
            └─ 출고(1) → Released 전환 + current_stock 차감 + 결과 출력
                  true 반환 (목록 재표시)
```

`menu_release_processing()`은 `release_process_list()`가 false를 반환할 때까지 루프를 반복한다. 한 건 처리 완료 후 목록이 자동으로 재표시된다.

---

## 5. Confirmed 목록 표시 및 선택 — `release_process_list` (FR-R-01~02)

### 5.1 데이터 로드

```cpp
order_repo_.refresh();
auto confirmed = order_repo_.find_by_status(Order::STATUS_CONFIRMED);
```

빈 경우: `"출고 가능한 주문이 없습니다.\n"` 출력 후 `false` 반환.

### 5.2 테이블 출력 (FR-R-01)

컬럼: `No`, `주문번호`, `고객`, `시료ID`, `수량(ea)`

`TablePrinter::print()` 사용 (Phase 4 Reserved 목록과 동일 방식).

### 5.3 번호 입력 (FR-R-02)

```cpp
int choice = InputUtil::read_int(
    "번호를 입력하세요 (0: 돌아가기): ",
    0, static_cast<int>(confirmed.size()));

if (choice == 0) return false;

release_process_detail(confirmed[static_cast<std::size_t>(choice - 1)]);
return true;
```

---

## 6. 출고/취소 선택 및 처리 — `release_process_detail` (FR-R-03~06)

### 6.1 주문 정보 출력

```
[ 출고 확인 ]
  주문번호: ORD-20240501-003
  고객    : 홍길동
  시료 ID : S-001
  수량    : 100 ea
```

### 6.2 출고/취소 선택 (FR-R-03~04)

```cpp
std::cout << "\n 1. 출고  0. 취소\n";
int action = InputUtil::read_int("> ", 0, 1);
if (action == 0) return;  // 취소 → 목록 복귀
```

### 6.3 출고 처리 (FR-R-05)

**① released_at 기록**

```cpp
std::string released_at = Timestamp::now();  // "YYYY-MM-DD HH:MM:SS" UTC
```

**② 주문 상태 Released 전환 + released_at 저장**

```cpp
JsonValue upd_order = JsonValue::object();
upd_order["order_status"] = JsonValue(int64_t(Order::STATUS_RELEASED));
upd_order["released_at"]  = JsonValue(released_at);
order_store_.update(order.id, upd_order);
order_repo_.refresh();
```

**③ current_stock 차감**

시료의 최신 재고를 파일에서 다시 읽어 차감한다(cached 값 사용 금지).

```cpp
sample_repo_.refresh();
auto sample_opt = sample_repo_.find_by_sample_id(order.sample_id);
if (!sample_opt.has_value()) {
    std::cout << "시료 정보를 찾을 수 없습니다.\n";
    return;
}
const Sample& sample = sample_opt.value();
int64_t new_stock = sample.current_stock - order.order_quantity;

JsonValue upd_sample = JsonValue::object();
upd_sample["current_stock"] = JsonValue(new_stock);
sample_store_.update(sample.id, upd_sample);
sample_repo_.refresh();
```

> `current_stock`이 `order_quantity`보다 작아 음수가 되는 경우는 정상 운영 시 발생하지 않는다. Phase 4에서 Confirmed 전환 시점에 재고 충분이 보장됐거나, Phase 8 자동 전환 시 `actual_production`이 stock에 가산되었기 때문이다. 방어적 검사는 추가하지 않는다.

### 6.4 결과 출력 (FR-R-06)

```
출고 처리가 완료되었습니다.
  주문번호: ORD-20240501-003
  출고수량: 100 ea
  처리일시: 2024-05-01 14:30:00
  상태    : Confirmed → Released
```

`released_at` 문자열을 그대로 출력한다 (`"YYYY-MM-DD HH:MM:SS"` 형식).

---

## 7. FR-U-04 (빈 줄 취소) 적용 범위

출고 처리 흐름의 입력은 모두 정수 선택(`InputUtil::read_int`)이다. 자유 텍스트 입력 프롬프트가 없으므로 이 Phase에서 FR-U-04가 적용되는 입력 항목은 없다.

---

## 8. 전체 구현 스니펫

### menu_release_processing()

```cpp
void App::menu_release_processing() {
    while (release_process_list()) {
        // 한 건 처리 후 목록 재표시
    }
}
```

### release_process_list()

```cpp
bool App::release_process_list() {
    order_repo_.refresh();
    auto confirmed = order_repo_.find_by_status(Order::STATUS_CONFIRMED);

    if (confirmed.empty()) {
        std::cout << "출고 가능한 주문이 없습니다.\n";
        return false;
    }

    TablePrinter tp({"No", "주문번호", "고객", "시료ID", "수량(ea)"});
    for (std::size_t i = 0; i < confirmed.size(); ++i) {
        const auto& o = confirmed[i];
        tp.add_row({
            std::to_string(i + 1),
            o.order_number,
            o.customer_name,
            o.sample_id,
            std::to_string(o.order_quantity)
        });
    }
    tp.print();

    int choice = InputUtil::read_int(
        "번호를 입력하세요 (0: 돌아가기): ",
        0, static_cast<int>(confirmed.size()));

    if (choice == 0) return false;

    release_process_detail(confirmed[static_cast<std::size_t>(choice - 1)]);
    return true;
}
```

### release_process_detail()

```cpp
void App::release_process_detail(const Order& order) {
    std::cout << "\n[ 출고 확인 ]\n"
              << "  주문번호: " << order.order_number  << "\n"
              << "  고객    : " << order.customer_name << "\n"
              << "  시료 ID : " << order.sample_id     << "\n"
              << "  수량    : " << order.order_quantity << " ea\n\n"
              << " 1. 출고  0. 취소\n";

    int action = InputUtil::read_int("> ", 0, 1);
    if (action == 0) return;

    std::string released_at = Timestamp::now();

    // 주문 상태 업데이트
    JsonValue upd_order = JsonValue::object();
    upd_order["order_status"] = JsonValue(int64_t(Order::STATUS_RELEASED));
    upd_order["released_at"]  = JsonValue(released_at);
    order_store_.update(order.id, upd_order);
    order_repo_.refresh();

    // 재고 차감
    sample_repo_.refresh();
    auto sample_opt = sample_repo_.find_by_sample_id(order.sample_id);
    if (!sample_opt.has_value()) {
        std::cout << "시료 정보를 찾을 수 없습니다.\n";
        return;
    }
    const Sample& sample = sample_opt.value();
    JsonValue upd_sample = JsonValue::object();
    upd_sample["current_stock"] = JsonValue(sample.current_stock - order.order_quantity);
    sample_store_.update(sample.id, upd_sample);
    sample_repo_.refresh();

    std::cout << "출고 처리가 완료되었습니다.\n"
              << "  주문번호: " << order.order_number  << "\n"
              << "  출고수량: " << order.order_quantity << " ea\n"
              << "  처리일시: " << released_at          << "\n"
              << "  상태    : Confirmed → Released\n";
}
```

---

## 9. 테스트 범위

`tests/mvc/test_release_processing.cpp` — 테스트 그룹 TG-RL

| TC | 시나리오 |
|----|---------|
| TC-RL-01 | Confirmed 주문 없을 때 안내 메시지 출력 후 반환 |
| TC-RL-02 | 출고 선택 → order_status = Released 전환 확인 |
| TC-RL-03 | 출고 선택 → released_at 기록 확인 (null 이 아닌 타임스탬프) |
| TC-RL-04 | 출고 선택 → current_stock -= order_quantity 확인 |
| TC-RL-05 | 취소(0) 선택 → order_status 미변경, current_stock 미변경 확인 |
| TC-RL-06 | 결과 출력에 주문번호·출고수량·처리일시·상태변화 포함 확인 (FR-R-06) |
| TC-RL-07 | 출고 후 목록 재조회 시 해당 주문이 Confirmed 목록에서 제거 확인 |
| TC-RL-08 | current_stock은 출고 직전 최신 파일 값 기준으로 차감 확인 |
