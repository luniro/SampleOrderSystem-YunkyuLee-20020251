# 테스트 시트

> 근거 문서: docs/PLAN.md, docs/PRD.md, lib/persistence/DATA_SCHEMA.md, lib/json/JSON_API.md

---

## 테스트 데이터셋

### Phase 0-1 테스트용 레코드 정의

Phase 0-1은 `DataStore`(lib/persistence)의 null 포함 신규 필드(`approved_at`, `released_at`, `production_start_at`) 저장·조회 검증이 목적이다. 아래 표준 레코드를 TC의 사전 조건 또는 입력값으로 사용한다.

#### Order 기준 레코드

| 식별자 | 필드 | 값 |
|--------|------|----|
| `ORD-A` | order_number | "ORD-20240501-001" |
|  | sample_id | "SMP-001" |
|  | customer_name | "홍길동" |
|  | order_quantity | 50 |
|  | order_status | 2 (Producing) |
|  | approved_at | "2024-05-01 10:30:00" |
|  | released_at | null |
| `ORD-B` | order_number | "ORD-20240501-002" |
|  | sample_id | "SMP-001" |
|  | customer_name | "김철수" |
|  | order_quantity | 30 |
|  | order_status | 0 (Reserved) |
|  | approved_at | null |
|  | released_at | null |
| `ORD-C` | order_number | "ORD-20240501-003" |
|  | sample_id | "SMP-002" |
|  | customer_name | "이영희" |
|  | order_quantity | 20 |
|  | order_status | 4 (Released) |
|  | approved_at | "2024-05-01 09:00:00" |
|  | released_at | "2024-05-02 14:00:00" |

#### Production 기준 레코드

| 식별자 | 필드 | 값 |
|--------|------|----|
| `PRD-A` | order_number | "ORD-20240501-001" |
|  | sample_name | "산화철 나노입자" |
|  | order_quantity | 50 |
|  | shortage | 20 |
|  | actual_production | 23 |
|  | ordered_at | "2024-05-01 08:00:00" |
|  | estimated_completion | "03:30" |
|  | production_start_at | "2024-05-01 10:30:00" |
| `PRD-B` (null 포함) | order_number | "ORD-20240501-004" |
|  | sample_name | "탄소 나노튜브" |
|  | order_quantity | 100 |
|  | shortage | 100 |
|  | actual_production | 112 |
|  | ordered_at | "2024-05-01 11:00:00" |
|  | estimated_completion | "06:00" |
|  | production_start_at | null |

#### null 값 처리 공통 규칙 (DATA_SCHEMA.md 기준)

- `approved_at`: Reserved·Rejected 상태 → null, Producing·Confirmed·Released → 타임스탬프 문자열
- `released_at`: Released 이외 상태 → null, Released → 타임스탬프 문자열
- `production_start_at`: enqueue 미확정 상태 → null, enqueue 확정 후 → 타임스탬프 문자열

---

## 테스트 케이스

### TG-DS: DataStore — null 포함 신규 필드 저장·조회 (Phase 0-1)

#### 사전 조건 공통 사항

- `DataStore`를 대상 파일 경로로 초기화한다. 파일이 없으면 빈 스토어로 시작한다.
- 각 TC는 별도의 임시 파일(`orders_test.json`, `productions_test.json` 등)을 사용하거나 TC 시작 시 파일을 삭제하여 초기 상태를 보장한다.
- `JsonValue` 오브젝트 구성 시 null 값은 `JsonValue()` 또는 `JsonValue(nullptr)`로 생성한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-DS-01 | null 필드 포함 레코드 create 후 동일 id로 read 왕복 | 빈 DataStore 초기화 (`orders_test.json` 없음) | 1) `ORD-B` 레코드 구성 (approved_at=null, released_at=null) → `create()` 호출<br>2) 반환된 id로 `read()` 호출 | `read()` 결과의 `approved_at.is_null() == true`, `released_at.is_null() == true`, `order_number == "ORD-20240501-002"` | 정상 | |
| TC-DS-02 | string 필드와 null 필드가 혼재하는 레코드 create → read 왕복 | 빈 DataStore 초기화 | 1) `ORD-A` 레코드 구성 (approved_at="2024-05-01 10:30:00", released_at=null) → `create()` 호출<br>2) 반환된 id로 `read()` 호출 | `approved_at.as_string() == "2024-05-01 10:30:00"`, `released_at.is_null() == true` | 정상 | |
| TC-DS-03 | null 필드를 string으로 update 후 read (null→string) | 빈 DataStore에 `ORD-B` create 완료 (approved_at=null) | 1) `update(id, {"approved_at": "2024-05-01 12:00:00"})` 호출<br>2) 동일 id로 `read()` 호출 | `approved_at.is_null() == false`, `approved_at.as_string() == "2024-05-01 12:00:00"` | 정상 | |
| TC-DS-04 | string 필드를 null로 update 후 read (string→null) | 빈 DataStore에 `ORD-A` create 완료 (approved_at="2024-05-01 10:30:00") | 1) `update(id, {"approved_at": null})` 호출<br>2) 동일 id로 `read()` 호출 | `approved_at.is_null() == true` | 정상 | |
| TC-DS-05 | released_at: null→string update 후 read | 빈 DataStore에 `ORD-A` create 완료 (released_at=null) | 1) `update(id, {"released_at": "2024-05-03 09:00:00"})` 호출<br>2) 동일 id로 `read()` 호출 | `released_at.is_null() == false`, `released_at.as_string() == "2024-05-03 09:00:00"` | 정상 | |
| TC-DS-06 | released_at: string→null update 후 read | 빈 DataStore에 `ORD-C` create 완료 (released_at="2024-05-02 14:00:00") | 1) `update(id, {"released_at": null})` 호출<br>2) 동일 id로 `read()` 호출 | `released_at.is_null() == true` | 정상 | |
| TC-DS-07 | production_start_at null 포함 레코드 create → read 왕복 | 빈 DataStore 초기화 (`productions_test.json` 없음) | 1) `PRD-B` 레코드 구성 (production_start_at=null) → `create()` 호출<br>2) 반환된 id로 `read()` 호출 | `production_start_at.is_null() == true`, `order_number == "ORD-20240501-004"` | 정상 | |
| TC-DS-08 | production_start_at: null→string update 후 read | 빈 DataStore에 `PRD-B` create 완료 (production_start_at=null) | 1) `update(id, {"production_start_at": "2024-05-01 14:00:00"})` 호출<br>2) 동일 id로 `read()` 호출 | `production_start_at.is_null() == false`, `production_start_at.as_string() == "2024-05-01 14:00:00"` | 정상 | |
| TC-DS-09 | null 필드 포함 레코드 파일 저장 후 DataStore 재초기화(load) — 값 보존 검증 | 빈 DataStore에 `ORD-B` create 완료 (approved_at=null, released_at=null). 파일 `orders_test.json` 존재 | 1) 동일 파일 경로로 DataStore를 새로 생성(재초기화)<br>2) 저장 시 반환된 id로 `read()` 호출 | `approved_at.is_null() == true`, `released_at.is_null() == true`, `order_number == "ORD-20240501-002"`. JSON 파일 내 해당 필드가 `null` 리터럴로 저장되어 있을 것 | 정상 | |
| TC-DS-10 | string 필드 포함 레코드 파일 저장 후 재초기화 — 값 보존 검증 | 빈 DataStore에 `ORD-A` create 완료 (approved_at="2024-05-01 10:30:00", released_at=null). 파일 존재 | 1) 동일 파일 경로로 DataStore 재초기화<br>2) 저장 시 반환된 id로 `read()` 호출 | `approved_at.as_string() == "2024-05-01 10:30:00"`, `released_at.is_null() == true` | 정상 | |
| TC-DS-11 | null 필드 포함 레코드를 update 후 파일 저장 → 재초기화 → read — 값 보존 검증 | 빈 DataStore에 `ORD-B` create, 이후 `approved_at`을 "2024-05-01 12:00:00"으로 update 완료. 파일 존재 | 1) DataStore 재초기화<br>2) 해당 id로 `read()` 호출 | `approved_at.as_string() == "2024-05-01 12:00:00"` (update 결과가 파일에 반영되고, 재로드 후에도 유지) | 정상 | |
| TC-DS-12 | read_all 결과에 null 필드 포함 레코드 포함 여부 확인 | 빈 DataStore에 `ORD-A`(approved_at=string), `ORD-B`(approved_at=null), `ORD-C`(released_at=string) 순서로 create 완료 (총 3건) | `read_all()` 호출 | 반환 벡터 크기 == 3. `ORD-B`에 해당하는 레코드의 `approved_at.is_null() == true`. 세 레코드 모두 포함 | 정상 | |
| TC-DS-13 | read_all 결과에서 null 필드 값 정확성 확인 | 빈 DataStore에 `ORD-A`(approved_at="2024-05-01 10:30:00", released_at=null) create | `read_all()` 호출 후 반환된 벡터의 첫 번째 원소 확인 | `approved_at.as_string() == "2024-05-01 10:30:00"`, `released_at.is_null() == true` | 정상 | |
| TC-DS-14 | null 필드 포함 레코드 remove 후 read_all에서 제거 확인 | 빈 DataStore에 `ORD-A`, `ORD-B` create (ORD-B는 approved_at=null). ORD-B의 id를 기록 | 1) ORD-B의 id로 `remove()` 호출<br>2) `read_all()` 호출 | `remove()` 정상 반환. `read_all()` 결과 크기 == 1. ORD-B 레코드 없음 | 정상 | |
| TC-DS-15 | null 필드 포함 레코드 remove 후 동일 id로 read 시 예외 | 빈 DataStore에 `ORD-B`(approved_at=null) create 후 remove 완료 | remove에 사용한 id로 `read()` 호출 | `RecordNotFoundError` 예외 발생. 예외 메시지에 해당 id 포함 | 이상 | |
| TC-DS-16 | 존재하지 않는 id로 read 시 RecordNotFoundError | 빈 DataStore 초기화 (레코드 없음) | `read(999)` 호출 | `RecordNotFoundError` 예외 발생. `what()` 문자열에 "999" 포함 | 이상 | |
| TC-DS-17 | 존재하지 않는 id로 update 시 RecordNotFoundError | 빈 DataStore 초기화 (레코드 없음) | `update(999, {"approved_at": "2024-05-01 10:30:00"})` 호출 | `RecordNotFoundError` 예외 발생. `what()` 문자열에 "999" 포함 | 이상 | |
| TC-DS-18 | 존재하지 않는 id로 remove 시 RecordNotFoundError | 빈 DataStore 초기화 (레코드 없음) | `remove(999)` 호출 | `RecordNotFoundError` 예외 발생. `what()` 문자열에 "999" 포함 | 이상 | |
| TC-DS-19 | 레코드가 1건 있을 때 존재하지 않는 id로 read | 빈 DataStore에 `ORD-A` create 완료 (id=1 부여됨) | `read(2)` 호출 | `RecordNotFoundError` 예외 발생 | 이상 | |
| TC-DS-20 | 레코드가 1건 있을 때 존재하지 않는 id로 update | 빈 DataStore에 `ORD-A` create 완료 (id=1 부여됨) | `update(2, {"approved_at": null})` 호출 | `RecordNotFoundError` 예외 발생 | 이상 | |
| TC-DS-21 | 레코드가 1건 있을 때 존재하지 않는 id로 remove | 빈 DataStore에 `ORD-A` create 완료 (id=1 부여됨) | `remove(2)` 호출 | `RecordNotFoundError` 예외 발생 | 이상 | |
| TC-DS-22 | update 시 id 필드는 변경 불가 | 빈 DataStore에 `ORD-A` create (id=1 부여됨) | `update(1, {"id": 999, "approved_at": "2024-05-01 11:00:00"})` 호출 후 `read(1)` | `read(1)` 정상 반환. 레코드의 `id == 1` (id 변경 안 됨). `approved_at.as_string() == "2024-05-01 11:00:00"` | 경계 | |
| TC-DS-23 | 빈 스토어에서 read_all 호출 시 빈 벡터 반환 | 빈 DataStore 초기화 (레코드 없음) | `read_all()` 호출 | 반환 벡터 크기 == 0. 예외 미발생 | 경계 | |
| TC-DS-24 | null 필드 포함 레코드에 대해 update로 추가 필드 삽입 | 빈 DataStore에 `ORD-B`(approved_at=null, released_at=null, order_status=0) create | `update(id, {"order_status": 1})` 호출 후 `read()` | `order_status.as_integer() == 1`. `approved_at.is_null() == true` (기존 null 필드 유지) | 정상 | |
| TC-DS-25 | production_start_at: string→null update 후 파일 저장·재로드 — null 보존 | 빈 DataStore에 `PRD-A`(production_start_at="2024-05-01 10:30:00") create. 파일 존재 | 1) `update(id, {"production_start_at": null})` 호출<br>2) DataStore 재초기화<br>3) 해당 id로 `read()` 호출 | `production_start_at.is_null() == true`. 파일 내 해당 필드가 `null` 리터럴로 저장 | 정상 | |

---

## 합계

| 그룹 | TC 수 |
|------|------|
| TG-DS | 25 |
| **합계** | **25** |
