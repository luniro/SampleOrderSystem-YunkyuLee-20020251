# SampleOrderSystem — Development Plan

> **참조**: [PRD.md](PRD.md) · [ARCHITECTURE.md](ARCHITECTURE.md) · [DATA_SCHEMA.md](../lib/persistence/DATA_SCHEMA.md) · [CLAUDE.md](../CLAUDE.md)  
> **최종 수정**: 2026-05-08

---

## Phase 0 — 라이브러리 기반 정비

DATA_SCHEMA.md 변경(`approved_at`, `released_at`, `production_start_at` 신규 필드)에 따라 기존 라이브러리를 순차적으로 수정한다. 본격 구현 전 빌드 기반을 정비하는 단계.

### 0-1 · `persistence` 검증

`DataStore`는 자유 형식 JSON이므로 스키마 필드 추가에 따른 코드 변경은 없다. 단, 신규 필드(`null` 포함)의 저장·조회가 정상 동작하는지 확인하고 미흡한 부분이 있으면 최소 범위로 보완한다.

### 0-2 · `dummy_generator` 수정

신규 필드를 포함하는 더미 데이터를 생성하도록 업데이트한다.

- `Order` 생성 시 상태(order_status)에 따른 `approved_at` 조건부 설정
  - `Reserved`, `Rejected` → `null`
  - `Producing`, `Confirmed`, `Released` → 임의 타임스탬프 (`"YYYY-MM-DD HH:MM:SS"`)
- `Order` 생성 시 `released_at` 설정
  - `Released` → 임의 타임스탬프 / 그 외 → `null`
- `Production` 생성 시 `production_start_at` 설정
  - enqueue 규칙(큐가 비면 `approved_at`, 선행 있으면 이전 완료시각)에 준하는 더미값 생성
- `Order.order_status` 열거형 `RELEASE` → `RELEASED` 오타 수정

### 0-3 · `monitor` 도메인 타입 수정

`src/monitor/domain/types.hpp` 의 도메인 구조체에 신규 필드를 추가하고, JSON 역직렬화 로직을 반영한다.

- `Order` 구조체: `approved_at` (string), `released_at` (string) 추가
- `Production` 구조체: `production_start_at` (string) 추가
- `OrderStatus` 열거형: `RELEASE` → `RELEASED` 수정
- 각 repository의 `from_json` 변환 로직에 신규 필드 매핑 추가

### 0-4 · 통합 빌드 및 MVC 셸 전환

현재 `src/mvc`는 할 일 관리 데모(TaskModel/TaskController)다. 이를 SampleOrderSystem의 메인 메뉴 셸로 교체하고, `monitor` 및 `persistence`와의 통합 빌드를 검증한다.

- `mvc` 내부 Task 관련 코드를 SampleOrderSystem 메뉴 구조로 교체
  - 메인 메뉴 6개 항목 + 종료(`0`) 루프
  - 각 메뉴 진입 시 `"준비 중"` stub 출력 (기능 미구현 상태)
- `App`이 `monitor`의 repository 및 `persistence`의 DataStore를 초기화하는 구조 확립
- 전체 빌드(`SampleOrderSystem.exe` + `DummyGenerator.exe`) 성공 확인

**산출물**: 메인 메뉴가 동작하는 바이너리 (각 메뉴 항목은 stub)

---

## Phase 1 — 기반 기능

본격 기능 구현 전 공통으로 필요한 유틸리티와 UI 기반을 마련한다.

- **테이블 행 paging**: `table_printer`에 행이 많을 경우 위아래로 이동하는 페이지 기능 추가. 페이지당 표시 행 수는 콘솔 높이를 동적으로 읽어 헤더·푸터·프롬프트 영역을 제외한 나머지로 결정. 콘솔 크기 조회는 별도 유틸리티로 래핑
- **입력 유효성 검사 유틸리티**: 정수 범위, 문자열 공백 등 공통 입력 검증 및 오류 시 재입력 루프 (FR-U-02)
- **타임스탬프 유틸리티**: 타임스탬프·duration 문자열 파싱·비교·포맷 함수 (`HH:MM (+N day(s))` 포맷 포함, FR-L-02, FR-L-08)
- **생산 계산 유틸리티**: 실 생산량 및 생산 소요 시간 산출 함수 (PRD 4.3 기준)

**산출물**: 기반 유틸리티가 갖춰진 동작 바이너리 (메뉴 stub 유지)

---

## Phase 2 — 시료 관리 (메뉴 1)

| 참조 FR | 목표 |
|---------|------|
| FR-S-01~03 | 시료 등록: 속성 입력 → 중복 속성 검사(시료 ID·시료명·수율·평균 생산시간) → 저장 → 부여된 ID 출력 |
| FR-S-04 | 시료 조회: 전체 시료 paged 테이블 출력 |
| FR-S-05~07, FR-S-06b | 시료 검색: 시료명 부분 검색(복수→테이블, 1건→상세) / 속성값 정확 검색(→단일 상세) / 0건 안내 |

**산출물**: 시료 관리(등록·조회·검색)가 동작하는 바이너리

---

## Phase 3 — 주문 접수 (메뉴 2)

| 참조 FR | 목표 |
|---------|------|
| FR-O-01~02 | 시료 ID 입력 및 등록 여부 검증 |
| FR-O-03~04 | 입력 요약 확인 화면, 취소 시 메뉴 복귀 |
| FR-O-05~06 | 주문번호 발행(`ORD-YYYYMMDD-NNN`), Reserved 저장, 결과 출력 |

**산출물**: 시료 관리 + 주문 접수가 동작하는 바이너리

---

## Phase 4 — 주문 처리 (메뉴 3)

> 핵심 비즈니스 로직이 집중된 단계. PRD 4.3의 재고 판정 Case 1/2 및 enqueue 규칙이 여기서 구현된다.

| 참조 FR | 목표 |
|---------|------|
| FR-A-01~02 | Reserved 주문 목록 표시, index 선택 |
| FR-A-03~04 | 재고 판정(PRD 4.3 Case 1/2): 가용 재고 산출, shortage 계산, 부족 시 실 생산량·소요 시간 표시 |
| FR-A-05~06 | 승인 / 거절 / 취소 선택 |
| FR-A-07 | 승인 처리: Confirmed 또는 Producing 전환. Producing 전환 시 생산 레코드 생성, `approved_at` 기록, `production_start_at` 확정(enqueue 규칙 적용) |
| FR-A-08~09 | 거절 처리: Rejected 전환. 처리 결과 출력 |

**산출물**: 주문 승인·거절 및 Producing/Confirmed 전환이 동작하는 바이너리

---

## Phase 5 — 모니터링 (메뉴 4)

> Phase 4에서 생성된 Producing/Confirmed 상태를 포함한 전체 주문·재고 현황 조회.

| 참조 FR | 목표 |
|---------|------|
| FR-M-01~02 | 주문량 확인: 상태별 건수(Reserved / Producing / Confirmed / Released) 표시 |
| FR-M-03~05 | 재고량 확인: 시료별 `current_stock` 및 재고 상태 판정(고갈 / 여유 / 부족) 테이블 출력 |

**산출물**: 모니터링이 동작하는 바이너리

---

## Phase 6 — 출고 처리 (메뉴 5)

| 참조 FR | 목표 |
|---------|------|
| FR-R-01~02 | Confirmed 주문 목록 표시, index 선택 |
| FR-R-03~04 | 출고 / 취소 선택 |
| FR-R-05 | 출고 처리: Released 전환, `current_stock -= order_quantity`, `released_at` 기록 |
| FR-R-06 | 처리 결과 출력: 주문번호, 출고수량, 처리일시, 상태 변화 |

**산출물**: 출고 처리가 동작하는 바이너리

---

## Phase 7 — 생산 라인 화면 (메뉴 6)

> 저장된 데이터 조회 기반의 표시 기능만 구현. 상태 자동 전환 로직은 Phase 8에서 별도 구현.

| 참조 FR | 목표 |
|---------|------|
| FR-L-01 | 생산 현황: 큐 최선두 주문 정보 표시(주문번호, 시료명, 주문량, 기존 가용재고, 부족분, 실 생산량, 수율, 총생산시간) |
| FR-L-02 | 진행률 바 그래프 + 완료 예정 `HH:MM (+N day(s))` 표시 |
| FR-L-03~05 | 대기 주문 테이블: 순서·주문번호·시료명·주문량·부족분·실 생산량·예상 완료시간(`production_start_at + estimated_completion` 직접 산출) |
| FR-L-06~08 | 빈 큐 안내 메시지, `HH:MM (+N day(s))` 포맷 적용 |

**산출물**: 생산 라인 화면이 동작하는 바이너리 (상태 자동 전환 미포함)

---

## Phase 8 — 백그라운드 자동 전환

> mvc Controller에서 담당. 생산 라인 화면(Phase 7)이 완성된 후 별도 구현.

| 참조 FR | 목표 |
|---------|------|
| FR-L-09 | 백그라운드 자동 전환: 완료 예정 시각(`production_start_at + estimated_completion`) 경과 시 Producing → Confirmed 전환, `current_stock += actual_production` |
| FR-L-10 | 재시작 lazy evaluation: 기동 시 `production_start_at` 순으로 Producing 주문을 순차 평가 → 경과 주문 일괄 Confirmed 전환 및 `current_stock += actual_production` |

**산출물**: 전체 기능이 완성된 바이너리

---

## Phase 9 — UI/UX 개선

> 기능 구현이 완료된 후 사용성과 일관성을 다듬는 단계. 기능 변경 없이 출력·흐름·피드백을 개선한다.

- 출력 포맷 일관성 정리: 헤더·구분선·여백·정렬 통일
- 오류 메시지 개선: 상황별 안내 문구 명확화, 재입력 흐름 자연스럽게 다듬기
- 메뉴 네비게이션 개선: 뒤로가기·취소 흐름 일관성 확보 (FR-U-03)
- 표 가독성 개선: 컬럼 너비·정렬·단위 표기 통일 (FR-U-01)
- 긴 목록 페이지 전환 UX: 안내 문구·키 표시 개선
- 상태 전환 결과 피드백: 성공·실패 메시지 시각적 구분 강화

**산출물**: UX가 개선된 최종 릴리스 바이너리
