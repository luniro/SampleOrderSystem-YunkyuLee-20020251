# Phase 3 상세 설계 — 주문 접수 (메뉴 2)

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

메인 메뉴 2번 "주문 접수"를 구현한다. 시료 ID·고객명·주문수량을 입력받고, 확인 화면을 거쳐 주문번호를 발행하고 `Reserved` 상태로 저장한다.

---

## 2. 파일 구조 변경

```
src/mvc/
├── App.hpp     ← 수정: order_store_ 멤버 추가, order_reception 메서드 추가
└── App.cpp     ← 수정: menu_order_reception() 구현
```

---

## 3. `App` 클래스 변경

### 3.1 멤버 추가

```cpp
// App.hpp — private 섹션
DataStore order_store_;   // 신규: 주문 쓰기용
```

### 3.2 생성자 initializer list 추가

```cpp
App::App(AppConfig config)
    : config_(std::move(config))
    , sample_store_(...)
    , order_store_((fs::path(config_.data_dir) / "orders.json").string())   // 신규
    , sample_repo_(...)
    , order_repo_(...)
    , production_repo_(...)
{}
```

### 3.3 메서드 추가

```cpp
// App.hpp private
void order_reception();   // 주문 입력·확인·저장 로직
```

---

## 4. 처리 흐름

```
menu_order_reception()
  └─ 서브 메뉴 없음 (단일 플로우)
       │
       ├─ 시료 ID 입력 → sample_repo_ 검증 (FR-O-02)
       │   실패: 오류 + 재입력
       │   성공: 시료명 확보
       │
       ├─ 고객명 입력
       │
       ├─ 주문수량 입력 (양의 정수)
       │
       ├─ 확인 화면 출력 (FR-O-03)
       │   ┌─────────────────────────────────┐
       │   │  시료: 시료명 (시료ID)          │
       │   │  고객: 고객명                   │
       │   │  수량: NNN ea                   │
       │   └─────────────────────────────────┘
       │   1. 접수 / 0. 취소
       │
       ├─ 취소(0): 메뉴로 복귀 (FR-O-04)
       │
       └─ 접수(1): 주문번호 발행 + Reserved 저장 + 결과 출력 (FR-O-05~06)
```

`menu_order_reception()`은 서브 메뉴 없이 `order_reception()`을 바로 호출한다.

---

## 5. 입력 항목 및 검증 규칙

| 항목 | 입력 방식 | 검증 조건 | 오류 시 |
|------|----------|-----------|---------|
| 시료 ID (`sample_id`) | `InputUtil::read_nonempty` | `sample_repo_` 에 존재할 것 (FR-O-02) | 오류 메시지 + 재입력 |
| 고객명 (`customer_name`) | `InputUtil::read_nonempty` | — | — |
| 주문수량 (`order_quantity`) | `InputUtil::read_int(1, INT_MAX)` | ≥ 1 | (read_int가 처리) |

---

## 6. 주문번호 발행 규칙 (FR-O-05)

형식: `ORD-YYYYMMDD-NNN`

- `YYYYMMDD`: 접수 시점의 **로컬 날짜**  
  (`std::time` + `localtime_s` / `localtime_r`)
- `NNN`: 해당 날짜의 기존 주문 수 + 1, 3자리 0-패딩  
  (999 초과 시 자릿수 확장 허용)
- 카운팅 방법: `order_repo_.refresh()` 후 `find_all()` 에서 `ORD-YYYYMMDD-` 접두사를 가진 주문 수를 셈

```cpp
// 오늘 날짜 문자열 "YYYYMMDD" 산출
std::time_t t = std::time(nullptr);
std::tm tm{};
#ifdef _WIN32
localtime_s(&tm, &t);
#else
localtime_r(&t, &tm);
#endif
char datebuf[9];
std::strftime(datebuf, sizeof(datebuf), "%Y%m%d", &tm);
std::string date_str = datebuf;

std::string prefix = "ORD-" + date_str + "-";
int count = 0;
order_repo_.refresh();
for (const auto& o : order_repo_.find_all())
    if (o.order_number.rfind(prefix, 0) == 0) ++count;

int seq = count + 1;
std::ostringstream oss;
oss << prefix;
if (seq < 1000) oss << std::setfill('0') << std::setw(3) << seq;
else            oss << seq;
std::string order_number = oss.str();
```

---

## 7. 저장 레코드

```cpp
JsonValue rec = JsonValue::object();
rec["order_number"]   = JsonValue(order_number);
rec["sample_id"]      = JsonValue(sample_id);
rec["customer_name"]  = JsonValue(customer_name);
rec["order_quantity"] = JsonValue(static_cast<int64_t>(quantity));
rec["order_status"]   = JsonValue(int64_t(Order::STATUS_RESERVED));
rec["approved_at"]    = JsonValue(nullptr);
rec["released_at"]    = JsonValue(nullptr);
order_store_.create(rec);
order_repo_.refresh();
```

---

## 8. 화면 출력

### 8.1 확인 화면 (FR-O-03)

```
[ 주문 확인 ]
  시료: 실리콘 웨이퍼 - 8인치 (S-001)
  고객: 홍길동
  수량: 100 ea

 1. 접수
 0. 취소
> 
```

`InputUtil::read_int("> ", 0, 1)` 사용.

### 8.2 접수 완료 출력 (FR-O-06)

```
주문이 접수되었습니다.
  주문번호: ORD-20240501-001
  상태    : Reserved
```

---

## 9. 테스트 범위

`tests/mvc/test_order_reception.cpp` — 테스트 그룹 TG-OR

| TC | 시나리오 |
|----|---------|
| TC-OR-01 | 정상 접수 → 주문번호 형식 확인 (`ORD-YYYYMMDD-NNN`) |
| TC-OR-02 | 접수 완료 후 orders.json에 Reserved 상태로 저장 확인 |
| TC-OR-03 | 존재하지 않는 시료 ID 입력 → 오류 메시지 후 재입력 (FR-O-02) |
| TC-OR-04 | 취소 선택 → 주문 미저장 확인 (FR-O-04) |
| TC-OR-05 | 주문수량 0 또는 음수 입력 → 오류 메시지 후 재입력 |
| TC-OR-06 | 동일 날짜 두 번째 접수 → NNN = 002 |
| TC-OR-07 | 확인 화면에 시료명·시료ID·고객명·수량 표시 확인 (FR-O-03) |
| TC-OR-08 | approved_at, released_at 이 null로 저장 확인 |
