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

### TG-DG: DummyGenerator — Phase 0-2 신규 필드 생성 검증

#### 사전 조건 공통 사항

- `GeneratorConfig`를 기본값(sample_count=5, order_count=10, production_count=8)으로 초기화하되, `output_dir`은 임시 디렉터리로 설정한다.
- 각 TC는 별도의 임시 디렉터리를 사용하거나 TC 시작 시 디렉터리를 삭제하여 초기 상태를 보장한다.
- 시드 42 고정: 동일 설정이면 항상 동일한 출력이 생성된다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-DG-01 | Reserved(status=0) 주문의 approved_at이 null | 기본 config로 generate_dummy_data 실행 | 생성된 orders.json에서 order_status==0 레코드 조회 | 해당 레코드의 `approved_at.is_null() == true` | 정상 | PASS |
| TC-DG-02 | Rejected(status=1) 주문의 approved_at이 null | 기본 config로 generate_dummy_data 실행 | 생성된 orders.json에서 order_status==1 레코드 조회 | 해당 레코드의 `approved_at.is_null() == true` | 정상 | PASS |
| TC-DG-03 | Producing(status=2) 주문의 approved_at이 타임스탬프 | 기본 config로 generate_dummy_data 실행 | 생성된 orders.json에서 order_status==2 레코드 조회 | `approved_at.is_null() == false`, 값이 `"YYYY-MM-DD HH:MM:SS"` 형식 | 정상 | PASS |
| TC-DG-04 | Confirmed(status=3) 주문의 approved_at이 타임스탬프 | 기본 config로 generate_dummy_data 실행 | 생성된 orders.json에서 order_status==3 레코드 조회 | `approved_at.is_null() == false`, 값이 `"YYYY-MM-DD HH:MM:SS"` 형식 | 정상 | PASS |
| TC-DG-05 | Released(status=4) 주문의 approved_at이 타임스탬프 | 기본 config로 generate_dummy_data 실행 | 생성된 orders.json에서 order_status==4 레코드 조회 | `approved_at.is_null() == false`, 값이 `"YYYY-MM-DD HH:MM:SS"` 형식 | 정상 | PASS |
| TC-DG-06 | 비-Released(status 0~3) 주문의 released_at이 null | 기본 config로 generate_dummy_data 실행 | 생성된 orders.json에서 order_status != 4 레코드 조회 | 해당 레코드의 `released_at.is_null() == true` | 정상 | PASS |
| TC-DG-07 | Released(status=4) 주문의 released_at이 타임스탬프 | 기본 config로 generate_dummy_data 실행 | 생성된 orders.json에서 order_status==4 레코드 조회 | `released_at.is_null() == false`, 값이 `"YYYY-MM-DD HH:MM:SS"` 형식 | 정상 | PASS |
| TC-DG-08 | order_status 값이 유효 범위 [0,4]에 속함 | 기본 config로 generate_dummy_data 실행 | 생성된 모든 orders 레코드의 order_status 확인 | 모든 레코드의 `order_status >= 0 && order_status <= 4` | 경계 | PASS |
| TC-DG-09 | production_start_at이 존재하고 타임스탬프 형식 | 기본 config로 generate_dummy_data 실행 | 생성된 productions.json의 모든 레코드 확인 | 모든 레코드에 `production_start_at` 필드 존재, 값이 `"YYYY-MM-DD HH:MM:SS"` 형식, null 아님 | 정상 | PASS |
| TC-DG-10 | production_start_at enqueue 연쇄 규칙 — 비단조 감소 없음 | 기본 config(production_count=8)로 generate_dummy_data 실행 | productions.json에서 i번째와 (i+1)번째 레코드의 production_start_at 비교 | `production_start_at[i+1] >= production_start_at[i]` (단조 비감소) | 정상 | PASS |
| TC-DG-11 | 시드 42 재현성 — 동일 설정 2회 실행 결과 동일 | 임시 디렉터리에 generate_dummy_data 실행 후 파일 삭제 | 동일 config로 2회 실행, 각 orders.json 로드 후 비교 | 두 실행의 레코드 수·order_status·approved_at 값이 완전히 일치 | 정상 | PASS |
| TC-DG-12 | 잘못된 config로 std::invalid_argument 예외 발생 | 임의 임시 디렉터리 | sample_count=0, order_count=-1, production_count=0 각각 시도 | 세 경우 모두 `std::invalid_argument` 발생 | 이상 | PASS |
| TC-DG-13 | 생성된 order 수가 config.order_count와 일치 | 기본 config(order_count=10) | generate_dummy_data 후 orders.json read_all() | 반환 벡터 크기 == 10 | 정상 | PASS |
| TC-DG-14 | 생성된 production 수가 config.production_count와 일치 | 기본 config(production_count=8) | generate_dummy_data 후 productions.json read_all() | 반환 벡터 크기 == 8 | 정상 | PASS |
| TC-DG-15 | 모든 order 레코드에 approved_at, released_at 필드 존재 | 기본 config | generate_dummy_data 후 모든 orders 레코드 확인 | `contains("approved_at") == true`, `contains("released_at") == true` | 정상 | PASS |
| TC-DG-16 | 모든 production 레코드에 production_start_at 필드 존재 | 기본 config | generate_dummy_data 후 모든 productions 레코드 확인 | `contains("production_start_at") == true` | 정상 | PASS |
| TC-DG-17 | ordered_at이 YYYY-MM-DD HH:MM:SS 형식 | 기본 config | generate_dummy_data 후 productions.json 레코드 확인 | 모든 `ordered_at`이 `"YYYY-MM-DD HH:MM:SS"` 형식 (기존 HH:MM 형식 아님) | 정상 | PASS |
| TC-DG-18 | 50건 order 생성 시 status 0~4 전체 출현 | order_count=50으로 generate_dummy_data 실행 | orders.json read_all() 후 status 집합 확인 | status 0·1·2·3·4가 모두 1회 이상 등장 | 경계 | PASS |
| TC-DG-19 | RELEASED(4) 주문의 approved_at·released_at 모두 설정 확인 | order_count=50으로 generate_dummy_data 실행 | status==4 레코드 확인 | `approved_at.is_null() == false`, `released_at.is_null() == false` 로 RELEASED 열거형 값이 4임을 간접 검증 | 정상 | PASS |
| TC-DG-20 | sample_count >= 1000 시 sample_id 형식 (n >= 1000 분기) | sample_count=1001로 generate_dummy_data 실행 | samples.json의 마지막 레코드 확인 | `sample_id == "S-1001"` (앞자리 0 없음) | 경계 | PASS |

---

### TG-MT: Monitor Domain Types — Phase 0-3 from_json 역직렬화 검증

#### 사전 조건 공통 사항

- `JsonValue` 오브젝트를 직접 구성하여 각 `from_json` 정적 메서드에 전달한다.
- null 값 필드는 `JsonValue()` 또는 `JsonValue(nullptr)`로 생성한다.
- 상수 정의 검증은 컴파일 타임 값을 런타임에서 확인한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-MT-01 | Sample::from_json — 모든 필드 정상 역직렬화 | 없음 | `id=1`, `sample_id="SMP-001"`, `sample_name="산화철 나노입자"`, `avg_production_time=4.5`, `yield_rate=0.87`, `current_stock=200` 으로 JsonValue 구성 후 `Sample::from_json()` 호출 | `s.id==1`, `s.sample_id=="SMP-001"`, `s.sample_name=="산화철 나노입자"`, `s.avg_production_time==4.5`, `s.yield_rate==0.87`, `s.current_stock==200` | 정상 | PASS |
| TC-MT-02 | Order::from_json — approved_at=string, released_at=null | 없음 | `order_status=2(Producing)`, `approved_at="2024-05-01 10:30:00"`, `released_at=null` 포함 JsonValue 구성 후 `Order::from_json()` 호출 | `o.order_status==2`, `o.approved_at=="2024-05-01 10:30:00"`, `o.released_at==""` (빈 문자열) | 정상 | PASS |
| TC-MT-03 | Order::from_json — approved_at=null, released_at=null (Reserved) | 없음 | `order_status=0(Reserved)`, `approved_at=null`, `released_at=null` 포함 JsonValue 구성 후 `Order::from_json()` 호출 | `o.order_status==0`, `o.approved_at==""` (빈 문자열), `o.released_at==""` (빈 문자열) | 정상 | PASS |
| TC-MT-04 | Order::from_json — approved_at=string, released_at=string (Released) | 없음 | `order_status=4(Released)`, `approved_at="2024-05-01 09:00:00"`, `released_at="2024-05-02 14:00:00"` 포함 JsonValue 구성 후 `Order::from_json()` 호출 | `o.order_status==4`, `o.approved_at=="2024-05-01 09:00:00"`, `o.released_at=="2024-05-02 14:00:00"` | 정상 | PASS |
| TC-MT-05 | Order::from_json — 기존 필드(order_number, sample_id, customer_name, order_quantity) 정상 역직렬화 | 없음 | `order_number="ORD-20240501-001"`, `sample_id="SMP-001"`, `customer_name="홍길동"`, `order_quantity=50`, `order_status=0`, `approved_at=null`, `released_at=null` 구성 | `o.order_number=="ORD-20240501-001"`, `o.sample_id=="SMP-001"`, `o.customer_name=="홍길동"`, `o.order_quantity==50` | 정상 | PASS |
| TC-MT-06 | Production::from_json — production_start_at=string 정상 역직렬화 | 없음 | `production_start_at="2024-05-01 10:30:00"` 포함한 완전한 Production JsonValue 구성 후 `Production::from_json()` 호출 | `p.production_start_at=="2024-05-01 10:30:00"` | 정상 | PASS |
| TC-MT-07 | Production::from_json — 모든 필드 정상 역직렬화 | 없음 | `id=1`, `order_number="ORD-20240501-001"`, `sample_name="산화철 나노입자"`, `order_quantity=50`, `shortage=20`, `actual_production=23`, `ordered_at="2024-05-01 08:00:00"`, `estimated_completion="03:30"`, `production_start_at="2024-05-01 10:30:00"` 구성 | 모든 필드가 입력값과 일치 | 정상 | PASS |
| TC-MT-08 | Order::STATUS_RESERVED == 0 | 없음 | `Order::STATUS_RESERVED` 값 확인 | `Order::STATUS_RESERVED == 0` | 경계 | PASS |
| TC-MT-09 | Order::STATUS_REJECTED == 1 | 없음 | `Order::STATUS_REJECTED` 값 확인 | `Order::STATUS_REJECTED == 1` | 경계 | PASS |
| TC-MT-10 | Order::STATUS_PRODUCING == 2 | 없음 | `Order::STATUS_PRODUCING` 값 확인 | `Order::STATUS_PRODUCING == 2` | 경계 | PASS |
| TC-MT-11 | Order::STATUS_CONFIRMED == 3 | 없음 | `Order::STATUS_CONFIRMED` 값 확인 | `Order::STATUS_CONFIRMED == 3` | 경계 | PASS |
| TC-MT-12 | Order::STATUS_RELEASED == 4 (RELEASE 오타 수정 확인) | 없음 | `Order::STATUS_RELEASED` 값 확인. `STATUS_RELEASE`가 아닌 `STATUS_RELEASED`로 명명됨을 확인 | `Order::STATUS_RELEASED == 4` | 경계 | PASS |
| TC-MT-13 | Order::from_json — id 필드 정상 역직렬화 | 없음 | `id=42` 포함 JsonValue 구성 후 `Order::from_json()` 호출 | `o.id==42` | 정상 | PASS |
| TC-MT-14 | OrderRepository::find_all — DataStore에 저장된 Order 레코드 조회 | 임시 파일에 ORD-A, ORD-B, ORD-C를 DataStore로 create 완료 | `OrderRepository(파일경로).find_all()` 호출 | 반환 벡터 크기==3, 각 레코드의 order_number가 입력과 일치, ORD-A의 approved_at=="2024-05-01 10:30:00", ORD-B의 approved_at=="" | 정상 | PASS |
| TC-MT-15 | OrderRepository::find_by_status — 상태별 필터 | 임시 파일에 ORD-A(status=2), ORD-B(status=0), ORD-C(status=4) create 완료 | `find_by_status(2)` 호출 | 반환 벡터 크기==1, `result[0].order_number=="ORD-20240501-001"` | 정상 | PASS |
| TC-MT-16 | OrderRepository::find_by_order_number — 존재하는 주문번호 조회 | 임시 파일에 ORD-A create 완료 | `find_by_order_number("ORD-20240501-001")` 호출 | 반환 optional has_value()==true, `value().approved_at=="2024-05-01 10:30:00"` | 정상 | PASS |
| TC-MT-17 | OrderRepository::find_by_order_number — 존재하지 않는 주문번호 조회 | 임시 파일에 ORD-A create 완료 | `find_by_order_number("ORD-NONEXISTENT")` 호출 | 반환 optional has_value()==false (nullopt) | 이상 | PASS |
| TC-MT-18 | ProductionRepository::find_all — 저장된 Production 레코드 조회 | 임시 파일에 PRD-A create 완료 | `ProductionRepository(파일경로).find_all()` 호출 | 반환 벡터 크기==1, `result[0].production_start_at=="2024-05-01 10:30:00"` | 정상 | PASS |
| TC-MT-19 | ProductionRepository::find_by_order_number — 존재하는 주문번호 조회 | 임시 파일에 PRD-A create 완료 | `find_by_order_number("ORD-20240501-001")` 호출 | 반환 optional has_value()==true, `value().production_start_at=="2024-05-01 10:30:00"` | 정상 | PASS |
| TC-MT-20 | ProductionRepository::find_by_order_number — 존재하지 않는 주문번호 조회 | 임시 파일에 PRD-A create 완료 | `find_by_order_number("ORD-NONEXISTENT")` 호출 | 반환 optional has_value()==false (nullopt) | 이상 | PASS |

---

## 합계

| 그룹 | TC 수 |
|------|------|
| TG-DS | 25 |
| TG-DG | 20 |
| TG-MT | 20 |
| **합계** | **65** |
