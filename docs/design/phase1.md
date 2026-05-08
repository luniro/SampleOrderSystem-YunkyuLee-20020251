# Phase 1 상세 설계 — 기반 유틸리티

> **참조**: [PLAN.md](../PLAN.md) · [PRD.md](../PRD.md) · [ARCHITECTURE.md](../ARCHITECTURE.md)  
> **작성일**: 2026-05-08

---

## 1. 개요

Phase 1은 Phase 2 이후의 기능 구현에 필요한 공통 유틸리티를 마련한다.  
기능 추가이며, 기존 코드의 동작은 변경하지 않는다.

### 산출물

| 유틸리티 | 위치 | 설명 |
|----------|------|------|
| 콘솔 크기 유틸리티 | `src/monitor/ui/console.hpp/cpp` | 콘솔 높이 조회 |
| 테이블 paging | `src/monitor/ui/table_printer` 확장 | 행 페이징 출력 |
| 입력 유효성 검사 | `src/mvc/input_util.hpp/cpp` | 정수·문자열 입력 검증 루프 |
| 타임스탬프 유틸리티 | `src/monitor/util/timestamp.hpp/cpp` | 파싱·포맷·완료 시각 표시 |
| 생산 계산 유틸리티 | `src/monitor/util/production_calc.hpp/cpp` | 실 생산량·소요 시간 산출 |

---

## 2. 파일 구조 변경

```
src/monitor/
├── util/                         ← 신규 디렉터리
│   ├── timestamp.hpp/cpp         ← 신규
│   └── production_calc.hpp/cpp  ← 신규
├── ui/
│   ├── console.hpp/cpp           ← 신규
│   ├── table_printer.hpp/cpp     ← 수정 (paging 추가)
│   └── app.hpp/cpp               ← 미수정
└── CMakeLists.txt                ← 수정 (신규 소스 추가)

src/mvc/
├── input_util.hpp/cpp            ← 신규
└── CMakeLists.txt                ← 수정 (input_util.cpp 추가)
```

---

## 3. 콘솔 크기 유틸리티 (`src/monitor/ui/console.hpp`)

### 목적

테이블 paging에서 페이지당 행 수를 동적으로 결정하기 위해 콘솔 높이를 조회한다.

### API

```cpp
// console.hpp
#pragma once

namespace Console {
    // 현재 콘솔 높이(줄 수)를 반환한다. 조회 실패 시 24를 반환한다.
    int get_height();
}
```

### 구현 규칙

- Windows: `GetConsoleScreenBufferInfo` 사용 (`<windows.h>`)
- 조회 실패(비터미널 환경, 리다이렉션 등): fallback 값 `24` 반환

---

## 4. 테이블 paging (`src/monitor/ui/table_printer.hpp`)

### 목적

행이 많은 경우 한 화면에 모두 출력하지 않고 페이지 단위로 나눠서 탐색할 수 있도록 한다.

### API 변경

기존 `print()` 유지. `print_paged()` 추가.

```cpp
// table_printer.hpp
class TablePrinter {
public:
    explicit TablePrinter(std::vector<std::string> headers);

    void add_row(std::vector<std::string> row);
    void print() const;           // 기존 — 전체 출력 (paging 없음)
    void print_paged() const;     // 신규 — 콘솔 높이 기반 paging
};
```

### `print_paged()` 동작 규칙

1. `Console::get_height()` 로 콘솔 높이 H를 조회한다.
2. 페이지당 표시 가능 행 수 = H − RESERVED_ROWS (헤더 1 + 구분선 1 + 프롬프트 1 + 여유 1 = 4)
   - 최솟값: 1 (음수·0 방지)
3. 전체 행이 한 페이지 이하이면 `print()` 와 동일하게 전체 출력 후 반환한다.
4. 여러 페이지이면 현재 페이지를 출력한 뒤 아래 프롬프트를 표시한다.

```
[n]다음  [p]이전  [0]나가기  (1/3):
```

5. 입력 처리:
   - `n` 또는 빈 입력(Enter): 다음 페이지. 마지막 페이지에서 `n`은 무시한다.
   - `p`: 이전 페이지. 첫 페이지에서 `p`는 무시한다.
   - `0`: paging 루프 종료.
   - 그 외: 무시하고 재출력하지 않음 (프롬프트만 재표시).

---

## 5. 입력 유효성 검사 유틸리티 (`src/mvc/input_util.hpp`)

### 목적

메뉴 선택·수량·ID 입력 등 사용자 입력을 검증하고, 잘못된 입력 시 재입력을 유도한다 (FR-U-02).

### API

```cpp
// input_util.hpp
#pragma once
#include <string>

namespace InputUtil {
    // prompt 출력 후 정수를 읽는다.
    // [min_val, max_val] 범위를 벗어나거나 정수가 아닌 경우 error_msg를 출력하고 재입력을 유도한다.
    // error_msg 기본값: "잘못된 입력입니다. 다시 입력해 주세요."
    int read_int(const std::string& prompt,
                 int min_val,
                 int max_val,
                 const std::string& error_msg = "잘못된 입력입니다. 다시 입력해 주세요.");

    // prompt 출력 후 문자열을 읽는다.
    // 공백만으로 구성된 입력(빈 줄 포함)은 오류로 간주하고 재입력을 유도한다.
    std::string read_nonempty(const std::string& prompt,
                              const std::string& error_msg = "빈 값은 입력할 수 없습니다. 다시 입력해 주세요.");
}
```

### 구현 규칙

- `read_int`: `std::getline` 으로 읽은 후 `std::stoi` 변환 시도. 변환 실패(`std::invalid_argument`, `std::out_of_range`) 또는 범위 초과 시 오류 메시지 출력 후 루프 재진입.
- `read_nonempty`: `std::getline` 으로 읽은 후 모든 문자가 `isspace` 이면 오류로 처리. 반환값은 원본 문자열(trim 없음).
- `std::cin` EOF(스트림 닫힘) 시 루프 탈출: `read_int`는 `min_val`, `read_nonempty`는 빈 문자열 반환.

---

## 6. 타임스탬프 유틸리티 (`src/monitor/util/timestamp.hpp`)

### 목적

- 타임스탬프 문자열 파싱 및 산술 연산
- `HH:MM (+N day(s))` 포맷 생성 (FR-L-02, FR-L-08)
- 생산 진행률 계산을 위한 경과 시간 산출

### API

```cpp
// timestamp.hpp
#pragma once
#include <cstdint>
#include <string>

namespace Timestamp {
    // "YYYY-MM-DD HH:MM:SS" → UTC epoch 초 (int64_t)
    // 파싱 실패 시 0 반환
    int64_t parse(const std::string& ts);

    // UTC epoch 초 → "YYYY-MM-DD HH:MM:SS"
    std::string format(int64_t epoch);

    // 현재 UTC 시각 → "YYYY-MM-DD HH:MM:SS"
    std::string now();

    // "HH:MM" duration 문자열 → 총 분 (int64_t)
    // 예: "03:30" → 210
    int64_t parse_duration_minutes(const std::string& hhmm);

    // production_start_at ("YYYY-MM-DD HH:MM:SS") +
    // estimated_completion ("HH:MM") → 완료 예정 시각 epoch 초
    int64_t completion_epoch(const std::string& production_start_at,
                             const std::string& estimated_completion);

    // 완료 예정 시각을 화면 표시용 문자열로 변환
    // - completion과 now가 같은 날짜(UTC 기준)인 경우: "HH:MM"
    // - 다음날 이후인 경우: "HH:MM (+N day(s))"
    //   N=1 → "+1 day", N≥2 → "+N days"
    // completion_epoch: 완료 예정 시각 epoch 초
    // now_epoch: 현재 시각 epoch 초
    std::string format_completion(int64_t completion_epoch, int64_t now_epoch);

    // 생산 진행률 [0.0, 100.0]
    // 경과 시간 / 전체 소요 시간 × 100, clamp [0, 100]
    // production_start_at: "YYYY-MM-DD HH:MM:SS"
    // estimated_completion: "HH:MM"
    // now_epoch: 현재 시각 epoch 초
    double calc_progress(const std::string& production_start_at,
                         const std::string& estimated_completion,
                         int64_t now_epoch);
}
```

### 구현 규칙

- 모든 시각은 UTC 기준으로 처리한다.
- Windows: `gmtime_s` 사용, Linux/macOS: `gmtime_r` 사용.
- `format_completion` 날짜 비교: epoch 초를 86400으로 나눈 몫(UTC 날짜 인덱스)의 차이로 N 산출.
  - `completion_day = completion_epoch / 86400`
  - `now_day = now_epoch / 86400`
  - `N = completion_day − now_day`
  - N ≤ 0이면 `"HH:MM"` (이미 경과하거나 당일)
- `calc_progress`: 분모(total_minutes)가 0이면 100.0 반환.

---

## 7. 생산 계산 유틸리티 (`src/monitor/util/production_calc.hpp`)

### 목적

PRD 4.3 기준에 따른 실 생산량 및 생산 소요 시간을 산출한다.

### API

```cpp
// production_calc.hpp
#pragma once
#include <cstdint>
#include <string>

namespace ProductionCalc {
    // 실 생산량: ceil(shortage / (yield_rate * 0.9))
    // shortage = 0 이면 0 반환
    int64_t actual_production(int64_t shortage, double yield_rate);

    // 총 생산 소요 시간(분): actual_production * avg_production_time(시간) * 60
    // avg_production_time 단위: 시간(h)
    double estimated_minutes(int64_t actual_production, double avg_production_time);

    // 총 분 → "HH:MM" 문자열
    // 예: 210.4 → "03:30" (반올림)
    std::string format_duration(double total_minutes);
}
```

### 구현 규칙

- `actual_production`: `std::ceil(static_cast<double>(shortage) / (yield_rate * 0.9))` 후 `int64_t` 캐스팅.
- `estimated_minutes`: `static_cast<double>(actual_production) * avg_production_time * 60.0`.
- `format_duration`: `std::round(total_minutes)` 후 `HH = total_mins / 60`, `MM = total_mins % 60`. 항상 2자리(`std::setfill('0')`, `std::setw(2)`).

---

## 8. CMakeLists.txt 변경 요약

### `src/monitor/CMakeLists.txt`

신규 소스 파일 추가:
```cmake
target_sources(monitor_lib PRIVATE
    ...
    ui/console.cpp
    util/timestamp.cpp
    util/production_calc.cpp
)
```

Windows API(`GetConsoleScreenBufferInfo`) 사용으로 별도 링크 불필요 (windows.h 자동 포함).

### `src/mvc/CMakeLists.txt`

신규 소스 파일 추가:
```cmake
target_sources(mvc PRIVATE
    ...
    input_util.cpp
)
```

---

## 9. 테스트 범위

각 유틸리티는 독립적으로 단위 테스트 가능하다.  
`tests/monitor/test_phase1_utils.cpp` 에 아래 테스트 그룹을 작성한다.

| 테스트 그룹 | 대상 | 주요 TC |
|-------------|------|---------|
| TG-TS | `Timestamp` | parse, format, parse_duration_minutes, completion_epoch, format_completion (당일/익일/+N days), calc_progress |
| TG-PC | `ProductionCalc` | actual_production (shortage=0/양수), estimated_minutes, format_duration |
| TG-IU | `InputUtil` | read_int (정상/범위초과/비정수/EOF), read_nonempty (정상/공백/빈줄/EOF) |
| TG-TP | `TablePrinter` (paging) | print_paged 전체행≤페이지/복수페이지/n·p·0 네비게이션 |

`tests/mvc/test_input_util.cpp` 에 TG-IU 작성.  
`tests/monitor/test_phase1_utils.cpp` 에 TG-TS, TG-PC 작성.  
`tests/monitor/test_table_paging.cpp` 에 TG-TP 작성.
