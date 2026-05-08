# Phase 7 상세 설계 — 생산 라인 화면 (메뉴 6)

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

메인 메뉴 6번 "생산 라인"을 구현한다. 현재 `menu_production_line()`은 `"준비 중"` stub이다. 상태 자동 전환은 Phase 8에서 구현하며, 이 단계는 **저장된 데이터 조회·표시만** 담당한다.

화면은 두 영역으로 구성된다.

- **생산 현황**: Producing 큐 최선두 주문 정보 + 진행률 바 + 완료 예정 시각 (FR-L-01~02, FR-L-06)
- **대기 주문**: 큐 2번째 이후 주문 테이블 (FR-L-03~05, FR-L-07~08)

데이터 쓰기 없음. 기존 멤버(`sample_repo_`, `order_repo_`, `production_repo_`)를 그대로 사용한다.

---

## 2. 파일 구조 변경

```
src/mvc/
├── App.hpp   ← 수정: production_line 서브 메서드·헬퍼 선언 추가
└── App.cpp   ← 수정: menu_production_line() 구현 + 서브 메서드 구현
```

신규 파일 없음.

---

## 3. `App` 클래스 변경

### 3.1 App.hpp — private 메서드 추가

```cpp
// 생산 라인 서브 메서드
void production_line_show();   // 전체 화면 구성 + 출력

// 헬퍼 (static)
static std::string make_progress_bar(double pct, int width = 20);
```

---

## 4. 처리 흐름

```
menu_production_line()
  └─ production_line_show()
       │
       ├─ 데이터 로드 + 큐 구성
       │   order_repo_.refresh()    → Producing 주문 수집 + approved_at 정렬
       │   production_repo_.refresh() → 주문번호로 Production 레코드 매핑
       │   sample_repo_.refresh()   → 수율 조회용
       │
       ├─ [ 생산 현황 ] 출력
       │   큐 비어있음 → "생산 중인 주문 없음" (FR-L-06)
       │   큐 존재    → 최선두 주문 상세 + 진행률 + 완료 예정 (FR-L-01~02)
       │
       ├─ [ 대기 주문 ] 출력
       │   대기 없음  → "대기 중인 주문 없음" (FR-L-07)
       │   대기 존재  → TablePrinter (FR-L-03~05)
       │
       └─ Enter 대기 후 메인 메뉴로 복귀
```

`menu_production_line()`은 루프 없이 `production_line_show()`를 한 번 호출한다.

---

## 5. 큐 구성 규칙 (FR-L-04)

```cpp
order_repo_.refresh();
std::vector<Order> producing;
for (const auto& o : order_repo_.find_all()) {
    if (o.order_status == Order::STATUS_PRODUCING)
        producing.push_back(o);
}

// 승인 시각 오름차순 정렬 (YYYY-MM-DD HH:MM:SS 포맷은 사전순 = 시간순)
std::sort(producing.begin(), producing.end(),
    [](const Order& a, const Order& b) { return a.approved_at < b.approved_at; });

production_repo_.refresh();
sample_repo_.refresh();
```

큐 인덱스:
- `producing[0]` → 생산 현황 (최선두)
- `producing[1..]` → 대기 주문

---

## 6. 생산 현황 출력 (FR-L-01~02)

### 6.1 빈 큐 (FR-L-06)

```
[ 생산 현황 ]
  생산 중인 주문 없음
```

### 6.2 현황 데이터 산출

```cpp
const Order& front       = producing[0];
auto prod_opt            = production_repo_.find_by_order_number(front.order_number);
auto sample_opt          = sample_repo_.find_by_sample_id(front.sample_id);

const Production& prod   = prod_opt.value();
int64_t available_stock  = prod.order_quantity - prod.shortage;  // 기존 가용재고
int    yield_pct         = static_cast<int>(sample_opt.value().yield_rate * 100);

int64_t now_epoch        = Timestamp::parse(Timestamp::now());
int64_t comp_epoch       = Timestamp::completion_epoch(prod.production_start_at,
                                                        prod.estimated_completion);
double  pct              = Timestamp::calc_progress(prod.production_start_at,
                                                     prod.estimated_completion,
                                                     now_epoch);
std::string comp_str     = Timestamp::format_completion(comp_epoch, now_epoch);
std::string bar          = make_progress_bar(pct);
```

`prod_opt` 또는 `sample_opt`가 없는 경우(데이터 불일치): `"시료/생산 정보를 찾을 수 없습니다."` 출력 후 해당 영역 스킵.

### 6.3 화면 출력 포맷

```
[ 생산 현황 ]
  주문번호    : ORD-20240501-002
  시료명      : 실리콘 웨이퍼 - 8인치
  주문량      : 200 ea
  기존 가용재고: 50 ea
  부족분      : 150 ea
  실 생산량   : 200 ea
  수율        : 75 %
  총생산시간  : 03:20
  진행률      : [████████████░░░░░░░░] 60 %
  완료 예정   : 14:30 (+1 day)
```

`기존 가용재고` = `prod.order_quantity − prod.shortage`  
`수율` = `static_cast<int>(yield_rate * 100)`  
`총생산시간` = `prod.estimated_completion` (이미 "HH:MM" 형식)

---

## 7. 진행률 바 헬퍼 — `make_progress_bar`

```cpp
// static
std::string App::make_progress_bar(double pct, int width) {
    int filled = static_cast<int>(pct / 100.0 * width);
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
        bar += (i < filled) ? u8"█" : u8"░";
    bar += "] ";
    bar += std::to_string(static_cast<int>(pct));
    bar += " %";
    return bar;
}
```

- 채움: `█` (U+2588), 빈칸: `░` (U+2591)
- `width` 기본값 20, 출력 예: `[████████░░░░░░░░░░░░] 40 %`
- `pct`는 `Timestamp::calc_progress`가 `[0, 100]`으로 clamp하므로 별도 방어 불필요

---

## 8. 대기 주문 출력 (FR-L-03~05, FR-L-07~08)

### 8.1 대기 없음 (FR-L-07)

```
[ 대기 주문 ]
  대기 중인 주문 없음
```

### 8.2 대기 주문 테이블

컬럼: `순서`, `주문번호`, `시료명`, `주문량(ea)`, `부족분(ea)`, `실 생산량(ea)`, `예상 완료시간`

```cpp
TablePrinter tp({"순서", "주문번호", "시료명", "주문량(ea)",
                 "부족분(ea)", "실 생산량(ea)", "예상 완료시간"});

for (std::size_t i = 1; i < producing.size(); ++i) {
    const Order& o   = producing[i];
    auto p_opt       = production_repo_.find_by_order_number(o.order_number);
    if (!p_opt.has_value()) continue;   // 데이터 불일치 → 행 스킵
    const Production& p = p_opt.value();

    int64_t comp_ep  = Timestamp::completion_epoch(p.production_start_at,
                                                    p.estimated_completion);
    std::string comp = Timestamp::format_completion(comp_ep, now_epoch);

    tp.add_row({
        std::to_string(i),                        // 순서: 1-based 대기 순번
        o.order_number,
        p.sample_name,
        std::to_string(p.order_quantity),
        std::to_string(p.shortage),
        std::to_string(p.actual_production),
        comp
    });
}
tp.print_paged();
```

`순서`는 대기 큐 내 순번(1부터). 전체 큐 순번(`i`)과 동일하며 `i = 1`이 대기 1번이다.

---

## 9. Enter 대기 (화면 일시 정지)

출력 완료 후 메인 메뉴 복귀 전 일시 정지한다.

```cpp
std::cout << "\n계속하려면 Enter를 누르세요...";
std::string dummy;
std::getline(std::cin, dummy);
```

---

## 10. 전체 구현 스니펫

### menu_production_line()

```cpp
void App::menu_production_line() {
    production_line_show();
}
```

### production_line_show()

```cpp
void App::production_line_show() {
    order_repo_.refresh();
    production_repo_.refresh();
    sample_repo_.refresh();

    // 큐 구성
    std::vector<Order> producing;
    for (const auto& o : order_repo_.find_all())
        if (o.order_status == Order::STATUS_PRODUCING)
            producing.push_back(o);
    std::sort(producing.begin(), producing.end(),
        [](const Order& a, const Order& b) { return a.approved_at < b.approved_at; });

    int64_t now_epoch = Timestamp::parse(Timestamp::now());

    // ── 생산 현황 ──────────────────────────────────────────────
    std::cout << "\n[ 생산 현황 ]\n";
    if (producing.empty()) {
        std::cout << "  생산 중인 주문 없음\n";
    } else {
        const Order& front  = producing[0];
        auto prod_opt       = production_repo_.find_by_order_number(front.order_number);
        auto sample_opt     = sample_repo_.find_by_sample_id(front.sample_id);

        if (!prod_opt.has_value() || !sample_opt.has_value()) {
            std::cout << "  시료/생산 정보를 찾을 수 없습니다.\n";
        } else {
            const Production& prod = prod_opt.value();
            int64_t avail  = prod.order_quantity - prod.shortage;
            int yield_pct  = static_cast<int>(sample_opt.value().yield_rate * 100);
            int64_t comp_epoch = Timestamp::completion_epoch(
                prod.production_start_at, prod.estimated_completion);
            double  pct    = Timestamp::calc_progress(
                prod.production_start_at, prod.estimated_completion, now_epoch);
            std::string comp_str = Timestamp::format_completion(comp_epoch, now_epoch);

            std::cout << "  주문번호    : " << front.order_number        << "\n"
                      << "  시료명      : " << prod.sample_name           << "\n"
                      << "  주문량      : " << prod.order_quantity        << " ea\n"
                      << "  기존 가용재고: " << avail                      << " ea\n"
                      << "  부족분      : " << prod.shortage              << " ea\n"
                      << "  실 생산량   : " << prod.actual_production     << " ea\n"
                      << "  수율        : " << yield_pct                  << " %\n"
                      << "  총생산시간  : " << prod.estimated_completion  << "\n"
                      << "  진행률      : " << make_progress_bar(pct)     << "\n"
                      << "  완료 예정   : " << comp_str                   << "\n";
        }
    }

    // ── 대기 주문 ──────────────────────────────────────────────
    std::cout << "\n[ 대기 주문 ]\n";
    if (producing.size() <= 1) {
        std::cout << "  대기 중인 주문 없음\n";
    } else {
        TablePrinter tp({"순서", "주문번호", "시료명", "주문량(ea)",
                         "부족분(ea)", "실 생산량(ea)", "예상 완료시간"});
        for (std::size_t i = 1; i < producing.size(); ++i) {
            const Order& o = producing[i];
            auto p_opt = production_repo_.find_by_order_number(o.order_number);
            if (!p_opt.has_value()) continue;
            const Production& p = p_opt.value();
            int64_t comp_ep = Timestamp::completion_epoch(
                p.production_start_at, p.estimated_completion);
            std::string comp = Timestamp::format_completion(comp_ep, now_epoch);
            tp.add_row({
                std::to_string(i),
                o.order_number,
                p.sample_name,
                std::to_string(p.order_quantity),
                std::to_string(p.shortage),
                std::to_string(p.actual_production),
                comp
            });
        }
        tp.print_paged();
    }

    std::cout << "\n계속하려면 Enter를 누르세요...";
    std::string dummy;
    std::getline(std::cin, dummy);
}
```

---

## 11. 테스트 범위

`tests/mvc/test_production_line.cpp` — 테스트 그룹 TG-PL

| TC | 시나리오 |
|----|---------|
| TC-PL-01 | Producing 주문 없음 → "생산 중인 주문 없음" 출력 (FR-L-06) |
| TC-PL-02 | Producing 1건만 → 생산 현황 출력, "대기 중인 주문 없음" 출력 (FR-L-07) |
| TC-PL-03 | Producing 3건 → 큐 순서가 approved_at 오름차순인지 확인 (FR-L-04) |
| TC-PL-04 | 생산 현황 출력: 주문번호·시료명·주문량·기존 가용재고·부족분·실 생산량·수율·총생산시간 포함 확인 (FR-L-01) |
| TC-PL-05 | 기존 가용재고 = order_quantity − shortage 계산 확인 |
| TC-PL-06 | 진행률 바 출력 확인 (FR-L-02): 0% → 전부 빈칸, 100% → 전부 채움 |
| TC-PL-07 | 완료 예정 포맷: 당일 → "HH:MM", 익일 이후 → "HH:MM (+N day(s))" (FR-L-02, FR-L-08) |
| TC-PL-08 | 대기 주문 테이블 컬럼: 순서·주문번호·시료명·주문량·부족분·실 생산량·예상 완료시간 (FR-L-03) |
| TC-PL-09 | 대기 주문 예상 완료시간 = production_start_at + estimated_completion 직접 산출 확인 (FR-L-05) |
| TC-PL-10 | 대기 주문 예상 완료시간 다음 날 이후 → "(+N day(s))" 포함 확인 (FR-L-08) |
| TC-PL-11 | make_progress_bar: pct=50.0, width=20 → 채움 10칸·빈칸 10칸 확인 |
