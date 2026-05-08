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

---

### TG-MVC: mvc App — Phase 0-4 메인 메뉴 셸 검증

#### 사전 조건 공통 사항

- `mvc::App`을 `AppConfig{data_dir = <임시 디렉터리>}`로 초기화한다.
- 각 TC는 `run()` 메서드를 표준입력 리디렉션(`std::istringstream` 등)으로 구동하여 출력을 `std::ostringstream`으로 캡처한다.
- 임시 디렉터리에 `samples.json`, `orders.json`, `productions.json` 파일은 없거나 빈 상태로 시작한다(DataStore는 파일 부재 시 빈 스토어로 시작).

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-MVC-01 | 메인 메뉴에 6개 항목 + 종료(0) 표시 | 임시 디렉터리, 빈 JSON 파일 없음 | 입력: "0\n" (즉시 종료) | 출력에 "1. 시료 관리", "2. 주문 접수", "3. 주문 처리", "4. 모니터링", "5. 출고 처리", "6. 생산 라인", "0. 종료" 모두 포함 | 정상 | |
| TC-MVC-02 | 메뉴 1 진입 시 "준비 중" 출력 | 임시 디렉터리 | 입력: "1\n0\n" (메뉴1 후 종료) | 출력에 "준비 중" 포함 | 정상 | |
| TC-MVC-03 | 메뉴 2 진입 시 시료 ID 프롬프트 출력 (Phase 3 구현 완료) | 임시 디렉터리 | 입력: "2\n" (메뉴2 진입 후 EOF) | 출력에 "시료 ID" 포함 (order_reception 프롬프트) | 정상 | |
| TC-MVC-04 | 메뉴 3 진입 시 "준비 중" 출력 | 임시 디렉터리 | 입력: "3\n0\n" (메뉴3 후 종료) | 출력에 "준비 중" 포함 | 정상 | |
| TC-MVC-05 | 메뉴 4 진입 시 모니터링 서브 메뉴 출력 (Phase 5 구현 완료) | 임시 디렉터리 | 입력: "4\n0\n0\n" (메뉴4 서브0 → 종료) | 출력에 "모니터링" 포함 | 정상 | PASS |
| TC-MVC-06 | 메뉴 5 진입 시 "준비 중" 출력 | 임시 디렉터리 | 입력: "5\n0\n" (메뉴5 후 종료) | 출력에 "준비 중" 포함 | 정상 | |
| TC-MVC-07 | 메뉴 6 진입 시 "준비 중" 출력 | 임시 디렉터리 | 입력: "6\n0\n" (메뉴6 후 종료) | 출력에 "준비 중" 포함 | 정상 | |
| TC-MVC-08 | 입력 "0"으로 프로그램 정상 종료 | 임시 디렉터리 | 입력: "0\n" | run() 정상 반환. 예외 미발생 | 정상 | |
| TC-MVC-09 | 잘못된 입력에 오류 메시지 출력 후 계속 | 임시 디렉터리 | 입력: "9\n0\n" (잘못된 입력 후 종료) | 출력에 "잘못된 입력" 포함. run() 정상 반환 | 이상 | |
| TC-MVC-10 | 여러 메뉴를 순서대로 진입 후 종료 (Phase 3 반영) | 임시 디렉터리 | 입력: "1\n0\n3\n0\n" (메뉴1 서브0 → 메뉴3 → 종료) | 출력에 "준비 중" 1회 이상 포함 (메뉴3 stub). run() 정상 반환 | 정상 | |

---

### TG-TS: Timestamp — Phase 1 타임스탬프 유틸리티 검증

#### 사전 조건 공통 사항

- `Timestamp` 네임스페이스 함수를 직접 호출한다.
- 모든 시각은 UTC 기준이다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-TS-01 | parse() — 유효한 타임스탬프 파싱 후 format()으로 역변환 일치 | 없음 | `parse("2024-05-01 10:30:00")` → epoch ep 획득, `format(ep)` 호출 | `format(ep) == "2024-05-01 10:30:00"` | 정상 | PASS |
| TC-TS-02 | parse() — 빈 문자열·잘못된 형식 → 0 반환 | 없음 | `parse("")`, `parse("not-a-date")` 각각 호출 | 두 결과 모두 == 0 | 이상 | PASS |
| TC-TS-03 | format() — epoch 0 → "1970-01-01 00:00:00" | 없음 | `format(0)` 호출 | `"1970-01-01 00:00:00"` | 경계 | PASS |
| TC-TS-04 | parse/format 왕복 일치 (연말 경계) | 없음 | `parse("2024-12-31 23:59:59")` → ep, `format(ep)` 호출 | `"2024-12-31 23:59:59"` | 경계 | PASS |
| TC-TS-05 | now() — 반환 문자열 형식 검증 | 없음 | `now()` 호출 | 크기==19, `[4]`, `[7]`=='-', `[10]`==' ', `[13]`, `[16]`==':' | 정상 | PASS |
| TC-TS-06 | parse_duration_minutes() — "03:30" → 210 | 없음 | `parse_duration_minutes("03:30")` 호출 | `210` | 정상 | PASS |
| TC-TS-07 | parse_duration_minutes() — "00:00" → 0 | 없음 | `parse_duration_minutes("00:00")` 호출 | `0` | 경계 | PASS |
| TC-TS-08 | parse_duration_minutes() — "06:00" → 360 | 없음 | `parse_duration_minutes("06:00")` 호출 | `360` | 정상 | PASS |
| TC-TS-09 | parse_duration_minutes() — 빈 문자열·잘못된 형식 → 0 | 없음 | `parse_duration_minutes("")`, `parse_duration_minutes("abc")` 호출 | 두 결과 모두 == 0 | 이상 | PASS |
| TC-TS-10 | completion_epoch() — 시작 + duration → 올바른 완료 epoch | 없음 | `completion_epoch("2024-05-01 10:30:00", "03:30")` 호출 | `format(result) == "2024-05-01 14:00:00"` | 정상 | PASS |
| TC-TS-11 | completion_epoch() — 자정 넘김 | 없음 | `completion_epoch("2024-05-01 22:00:00", "03:00")` 호출 | `format(result) == "2024-05-02 01:00:00"` | 경계 | PASS |
| TC-TS-12 | format_completion() — 당일(N==0) → "HH:MM" | 없음 | comp=`parse("2024-05-01 14:00:00")`, now=`parse("2024-05-01 10:00:00")` | `"14:00"` | 정상 | PASS |
| TC-TS-13 | format_completion() — 익일(N==1) → "HH:MM (+1 day)" | 없음 | comp=`parse("2024-05-02 02:00:00")`, now=`parse("2024-05-01 23:00:00")` | `"02:00 (+1 day)"` | 정상 | PASS |
| TC-TS-14 | format_completion() — 이틀 뒤(N==2) → "HH:MM (+2 days)" | 없음 | comp=`parse("2024-05-03 06:00:00")`, now=`parse("2024-05-01 10:00:00")` | `"06:00 (+2 days)"` | 정상 | PASS |
| TC-TS-15 | format_completion() — 이미 경과(N<=0) → "HH:MM" | 없음 | comp=`parse("2024-05-01 08:00:00")`, now=`parse("2024-05-01 10:00:00")` | `"08:00"` | 경계 | PASS |
| TC-TS-16 | calc_progress() — 진행 중 50% | 없음 | start="2024-05-01 00:00:00", now=start+30분, duration="01:00" | ≈ 50.0 | 정상 | PASS |
| TC-TS-17 | calc_progress() — 시작 전 → 0.0 (clamp) | 없음 | start="2024-05-01 12:00:00", now=start-60초, duration="01:00" | ≈ 0.0 | 경계 | PASS |
| TC-TS-18 | calc_progress() — 완료 후 → 100.0 (clamp) | 없음 | start="2024-05-01 00:00:00", now=start+2시간, duration="01:00" | ≈ 100.0 | 경계 | PASS |
| TC-TS-19 | calc_progress() — 분모 0(duration="00:00") → 100.0 | 없음 | duration="00:00", 임의 now | `100.0` | 경계 | PASS |

---

### TG-PC: ProductionCalc — Phase 1 생산 계산 유틸리티 검증

#### 사전 조건 공통 사항

- `ProductionCalc` 네임스페이스 함수를 직접 호출한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-PC-01 | actual_production() — shortage=0 → 0 | 없음 | `actual_production(0, 0.87)` 호출 | `0` | 경계 | PASS |
| TC-PC-02 | actual_production() — 일반 케이스: ceil(20/(0.87×0.9))=26 | 없음 | `actual_production(20, 0.87)` 호출 | `26` | 정상 | PASS |
| TC-PC-03 | actual_production() — 정확한 나누기: ceil(90/(1.0×0.9))=100 | 없음 | `actual_production(90, 1.0)` 호출 | `100` | 정상 | PASS |
| TC-PC-04 | actual_production() — shortage=1, 낮은 수율: ceil(1/0.45)=3 | 없음 | `actual_production(1, 0.5)` 호출 | `3` | 정상 | PASS |
| TC-PC-05 | estimated_minutes() — 일반 케이스: 23×4.5×60=6210 | 없음 | `estimated_minutes(23, 4.5)` 호출 | ≈ 6210.0 | 정상 | PASS |
| TC-PC-06 | estimated_minutes() — actual_production=0 → 0 | 없음 | `estimated_minutes(0, 4.5)` 호출 | ≈ 0.0 | 경계 | PASS |
| TC-PC-07 | format_duration() — 210.0 → "03:30" | 없음 | `format_duration(210.0)` 호출 | `"03:30"` | 정상 | PASS |
| TC-PC-08 | format_duration() — 210.4 → "03:30" (내림 반올림) | 없음 | `format_duration(210.4)` 호출 | `"03:30"` | 정상 | PASS |
| TC-PC-09 | format_duration() — 210.5 → "03:31" (반올림) | 없음 | `format_duration(210.5)` 호출 | `"03:31"` | 경계 | PASS |
| TC-PC-10 | format_duration() — 0.0 → "00:00" | 없음 | `format_duration(0.0)` 호출 | `"00:00"` | 경계 | PASS |
| TC-PC-11 | format_duration() — 60.0 → "01:00" | 없음 | `format_duration(60.0)` 호출 | `"01:00"` | 정상 | PASS |
| TC-PC-12 | format_duration() — 360.0 → "06:00" | 없음 | `format_duration(360.0)` 호출 | `"06:00"` | 정상 | PASS |
| TC-PC-13 | format_duration() — 대형 값 6210.0 → "103:30" | 없음 | `format_duration(6210.0)` 호출 | `"103:30"` | 경계 | PASS |

---

### TG-IU: InputUtil — Phase 1 입력 유효성 검사 유틸리티 검증

#### 사전 조건 공통 사항

- `std::cin` / `std::cout`을 `std::istringstream` / `std::ostringstream`으로 리디렉션하여 테스트한다.
- 각 TC는 테스트 종료 후 스트림을 원래대로 복원한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-IU-01 | read_int() — 유효 범위 내 정수 | cin="3\n" | `read_int("Enter: ", 1, 5)` 호출 | `3` | 정상 | PASS |
| TC-IU-02 | read_int() — 최솟값 경계 | cin="1\n" | `read_int("Enter: ", 1, 5)` 호출 | `1` | 경계 | PASS |
| TC-IU-03 | read_int() — 최댓값 경계 | cin="5\n" | `read_int("Enter: ", 1, 5)` 호출 | `5` | 경계 | PASS |
| TC-IU-04 | read_int() — 범위 초과 후 유효 입력 | cin="0\n6\n3\n" | `read_int("Enter: ", 1, 5)` 호출 | 반환값 `3`, 출력에 "잘못된 입력" 포함 | 이상 | PASS |
| TC-IU-05 | read_int() — 비정수 입력 후 유효 입력 | cin="abc\n2\n" | `read_int("Enter: ", 1, 5)` 호출 | 반환값 `2`, 출력에 "잘못된 입력" 포함 | 이상 | PASS |
| TC-IU-06 | read_int() — 빈 줄 후 유효 입력 | cin="\n4\n" | `read_int("Enter: ", 1, 5)` 호출 | 반환값 `4` | 이상 | PASS |
| TC-IU-07 | read_int() — EOF 즉시 min_val 반환 | cin="" (EOF) | `read_int("Enter: ", 1, 5)` 호출 | `1` (min_val) | 경계 | PASS |
| TC-IU-08 | read_int() — 커스텀 오류 메시지 출력 | cin="99\n3\n" | `read_int("Enter: ", 1, 5, "custom error")` 호출 | 출력에 "custom error" 포함 | 정상 | PASS |
| TC-IU-09 | read_int() — 실수 입력("3.5")은 비정수로 처리 | cin="3.5\n2\n" | `read_int("Enter: ", 1, 5)` 호출 | 반환값 `2`, 오류 메시지 출력 | 이상 | PASS |
| TC-IU-10 | read_nonempty() — 유효 문자열 | cin="hello\n" | `read_nonempty("Enter: ")` 호출 | `"hello"` | 정상 | PASS |
| TC-IU-11 | read_nonempty() — 빈 줄 후 유효 입력 | cin="\nhello\n" | `read_nonempty("Enter: ")` 호출 | `"hello"`, 출력에 "빈 값" 포함 | 이상 | PASS |
| TC-IU-12 | read_nonempty() — 공백만인 줄 후 유효 입력 | cin="   \nworld\n" | `read_nonempty("Enter: ")` 호출 | `"world"`, 출력에 "빈 값" 포함 | 이상 | PASS |
| TC-IU-13 | read_nonempty() — EOF 즉시 빈 문자열 반환 | cin="" (EOF) | `read_nonempty("Enter: ")` 호출 | `""` | 경계 | PASS |
| TC-IU-14 | read_nonempty() — 앞뒤 공백 있는 입력 trim 없이 반환 | cin="  hello  \n" | `read_nonempty("Enter: ")` 호출 | `"  hello  "` (원본 유지) | 정상 | PASS |
| TC-IU-15 | read_nonempty() — 커스텀 오류 메시지 출력 | cin="\nvalue\n" | `read_nonempty("Enter: ", "custom empty error")` 호출 | 출력에 "custom empty error" 포함 | 정상 | PASS |

---

### TG-TP: TablePrinter paging — Phase 1 페이지 탐색 검증

#### 사전 조건 공통 사항

- `std::cin` / `std::cout`을 리디렉션하여 테스트한다.
- 테스트 환경에서 `Console::get_height()`는 fallback 24를 반환하므로 page_rows = 20이다.
- 25행 테이블로 2페이지(20+5) 시나리오를 구성한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-TP-01 | print_paged() — 0행 → 헤더만 출력, 프롬프트 없음 | 빈 TablePrinter | cin="0\n" | 출력에 "Col" 포함, "[n]" 없음 | 경계 | PASS |
| TC-TP-02 | print_paged() — 전체 행 ≤ page_rows → 전체 출력, 프롬프트 없음 | 5행 TablePrinter | cin="" | 5행 모두 출력, "[n]" 없음 | 정상 | PASS |
| TC-TP-03 | print_paged() — "0" 입력으로 첫 페이지에서 종료 | 25행 TablePrinter | cin="0\n" | 프롬프트 "(1/2)" 출력, row_0~row_19 표시, row_20 없음 | 정상 | PASS |
| TC-TP-04 | print_paged() — "n"으로 다음 페이지 이동 | 25행 TablePrinter | cin="n\n0\n" | row_20~row_24 표시, "(2/2)" 출력 | 정상 | PASS |
| TC-TP-05 | print_paged() — 빈 입력(Enter)은 "n"과 동일 | 25행 TablePrinter | cin="\n0\n" | row_20~row_24 표시 | 정상 | PASS |
| TC-TP-06 | print_paged() — 첫 페이지에서 "p" 무시 | 25행 TablePrinter | cin="p\n0\n" | "(1/2)"가 2회 출력 (위치 이동 없음) | 경계 | PASS |
| TC-TP-07 | print_paged() — 마지막 페이지에서 "n" 무시 | 25행 TablePrinter | cin="n\nn\n0\n" | "(2/2)"가 2회 출력 | 경계 | PASS |
| TC-TP-08 | print_paged() — "n" 후 "p"로 첫 페이지 복귀 | 25행 TablePrinter | cin="n\np\n0\n" | "(1/2)" 첫 출력 → "(2/2)" → "(1/2)" 두 번째 출력 순서 | 정상 | PASS |
| TC-TP-09 | print_paged() — EOF에서 루프 종료 | 25행 TablePrinter | cin="" (EOF) | 크래시·무한루프 없이 반환. "[n]" 출력 후 종료 | 경계 | PASS |
| TC-TP-10 | print_paged() — 인식 불가 입력은 무시하고 동일 페이지 재출력 | 25행 TablePrinter | cin="x\n0\n" | "(1/2)"가 2회 출력 | 이상 | PASS |

---

---

### TG-SM: 시료 관리 — Phase 2 (메뉴 1: 등록·조회·검색) 검증

#### 사전 조건 공통 사항

- `mvc::App`을 `AppConfig{data_dir = <임시 디렉터리>}`로 초기화한다.
- 각 TC는 별도의 임시 디렉터리를 사용하여 독립적인 초기 상태를 보장한다.
- `std::cin` / `std::cout`을 `std::istringstream` / `std::ostringstream`으로 리디렉션하여 `App::run()`을 구동한다.
- 임시 디렉터리에 초기 JSON 파일 없음 (DataStore는 파일 부재 시 빈 스토어로 시작).

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-SM-01 | 정상 등록 → 조회로 확인 | 빈 DB | 메인1 → 서브1(등록): S-001·실리콘웨이퍼·1.5·0.90·350 → 서브2(조회) → 0·0 | "시료가 등록되었습니다", "S-001", "실리콘웨이퍼" 포함 | 정상 | PASS |
| TC-SM-02 | 중복 시료 ID 등록 거부 | 빈 DB | 첫 등록(S-001) 성공 후 동일 ID(S-001) 재입력 시도 → 오류 → S-002 재입력 성공 | "이미 존재하는 시료 ID" 및 "S-002" 포함 | 이상 | PASS |
| TC-SM-03 | avg_production_time 비정수·음수 입력 → 오류 후 재입력 | 빈 DB | "abc" 입력 → 오류, "-1.0" 입력 → 오류, "0.0" 입력 → 오류, "3.0" 입력 → 성공 | "유효하지 않은 값", "0보다 커야" 및 "시료가 등록되었습니다" 포함 | 이상 | PASS |
| TC-SM-04 | yield_rate 범위 초과 → 오류 후 재입력 | 빈 DB | "0.50" → 오류, "1.10" → 오류, "0.75" → 성공 | "0.70 이상 0.99 이하" 및 "시료가 등록되었습니다" 포함 | 이상 | PASS |
| TC-SM-05 | current_stock 음수 → 오류 후 재입력 | 빈 DB | "-1" → 오류, "0" → 성공 | "잘못된 입력" 및 "시료가 등록되었습니다" 포함 | 이상 | PASS |
| TC-SM-06 | 시료 조회 — 빈 DB에서 안내 메시지 | 빈 DB | 메인1 → 서브2(조회) → 0·0 | "등록된 시료가 없습니다" 포함 | 경계 | PASS |
| TC-SM-07 | 시료 조회 — 여러 시료 테이블 출력 확인 | 빈 DB | 2건 등록 후 조회 | "S-007A", "S-007B", "시료 ID"(헤더) 포함 | 정상 | PASS |
| TC-SM-08 | 시료명 부분 검색 — 0건 | 1건 등록 후 | 서브3→검색1: "갈륨"(없는 키워드) | "검색 결과가 없습니다" 포함 | 이상 | PASS |
| TC-SM-09 | 시료명 부분 검색 — 1건 → 상세 출력 | 1건 등록 후 | 서브3→검색1: "산화"(1건 일치) | "[ 시료 상세 ]", "S-009", "산화철나노" 포함 | 정상 | PASS |
| TC-SM-10 | 시료명 부분 검색 — 복수건 → 테이블 출력 | 2건 등록 후 | 서브3→검색1: "나노"(2건 일치) | "시료 ID"(헤더), "S-010A", "S-010B" 포함 | 정상 | PASS |
| TC-SM-11 | 시료 ID 검색 — 존재 → 상세 출력 | 1건 등록 후 | 서브3→검색2: "S-011" | "[ 시료 상세 ]", "S-011", "산화아연" 포함 | 정상 | PASS |
| TC-SM-12 | 시료 ID 검색 — 미존재 → 안내 메시지 | 1건 등록 후 | 서브3→검색2: "S-999" | "검색 결과가 없습니다" 포함 | 이상 | PASS |
| TC-SM-13 | 등록 완료 후 시료 ID 출력 확인 (FR-S-03) | 빈 DB | 1건 등록 (S-013) | "시료가 등록되었습니다", "S-013" 포함 | 정상 | PASS |
| TC-SM-14 | 중복 시료명 등록 거부 | 빈 DB | 첫 등록("이름중복") 후 동일 시료명 재시도 → 오류 → 새 이름으로 성공 | "이미 존재하는 시료명", "S-014B" 포함 | 이상 | PASS |
| TC-SM-15 | 중복 avg_production_time 등록 거부 | 빈 DB | 첫 등록(7.5h) 후 동일 값 재시도 → 오류 → 8.0h로 성공 | "이미 존재하는 평균 생산시간" 및 "시료가 등록되었습니다" 포함 | 이상 | PASS |
| TC-SM-16 | 중복 yield_rate 등록 거부 | 빈 DB | 첫 등록(0.92) 후 동일 값 재시도 → 오류 → 0.93으로 성공 | "이미 존재하는 수율" 및 "시료가 등록되었습니다" 포함 | 이상 | PASS |
| TC-SM-17 | 수율 정확 검색 — 존재 → 상세 출력 | 1건 등록 후 | 서브3→검색3: "0.77" | "[ 시료 상세 ]", "S-017" 포함 | 정상 | PASS |
| TC-SM-18 | 수율 정확 검색 — 미존재 → 안내 메시지 | 1건 등록 후 | 서브3→검색3: "0.99"(없는 값) | "검색 결과가 없습니다" 포함 | 이상 | PASS |
| TC-SM-19 | 평균 생산시간 정확 검색 — 존재 → 상세 출력 | 1건 등록 후 | 서브3→검색4: "12.5" | "[ 시료 상세 ]", "S-019" 포함 | 정상 | PASS |
| TC-SM-20 | 평균 생산시간 정확 검색 — 미존재 → 안내 메시지 | 1건 등록 후 | 서브3→검색4: "99.9"(없는 값) | "검색 결과가 없습니다" 포함 | 이상 | PASS |

---

### TG-OR: 주문 접수 — Phase 3 (메뉴 2) 검증

#### 사전 조건 공통 사항

- `mvc::App`을 `AppConfig{data_dir = <임시 디렉터리>}`로 초기화한다.
- 각 TC는 별도의 임시 디렉터리를 사용하여 독립적인 초기 상태를 보장한다.
- `std::cin` / `std::cout`을 `std::istringstream` / `std::ostringstream`으로 리디렉션하여 `App::run()`을 구동한다.
- 시료 ID 검증 테스트 전, 메뉴 1을 통해 유효한 시료를 사전 등록한다.
- 주문번호 형식: `ORD-YYYYMMDD-NNN` (YYYYMMDD는 접수 시점 로컬 날짜, NNN은 3자리 0-패딩 시퀀스).

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-OR-01 | 정상 접수 → 주문번호 형식 확인 (`ORD-YYYYMMDD-NNN`) | 시료 1건(S-001) 등록 완료 | 메인2 → S-001 → 홍길동 → 100 → 1(접수) → 0(종료) | 출력에 `ORD-[0-9]{8}-[0-9]{3,}` 형식 주문번호 포함, "주문이 접수되었습니다" 포함 | 정상 | |
| TC-OR-02 | 접수 완료 후 orders.json에 Reserved 상태로 저장 확인 | 시료 1건(S-001) 등록 완료 | 메인2 → S-001 → 김철수 → 50 → 1(접수) → 0(종료) | orders.json에 레코드 1건, `order_status==0(Reserved)`, `customer_name=="김철수"`, `order_quantity==50` | 정상 | |
| TC-OR-03 | 존재하지 않는 시료 ID 입력 → 오류 메시지 후 재입력 (FR-O-02) | 시료 1건(S-001) 등록 완료 | 메인2 → INVALID-ID(오류) → S-001(재입력) → 이영희 → 30 → 1 → 0 | "존재하지 않는 시료 ID" 포함, 이후 "주문이 접수되었습니다" 포함 | 이상 | |
| TC-OR-04 | 취소 선택 → 주문 미저장 확인 (FR-O-04) | 시료 1건(S-001) 등록 완료 | 메인2 → S-001 → 박민준 → 20 → 0(취소) → 0(종료) | orders.json 없거나 레코드 0건 | 정상 | |
| TC-OR-05 | 주문수량 0 또는 음수 입력 → 오류 메시지 후 재입력 | 시료 1건(S-001) 등록 완료 | 메인2 → S-001 → 최지수 → 0(오류) → -1(오류) → 10(성공) → 1 → 0 | "잘못된 입력" 포함, 이후 "주문이 접수되었습니다" 포함 | 이상 | |
| TC-OR-06 | 동일 날짜 두 번째 접수 → NNN = 002 | 시료 1건(S-001) 등록, 첫 번째 주문 접수 완료 | 새 App 인스턴스로 메인2 → S-001 → 두번째고객 → 20 → 1 → 0 | 출력에 "-002" 포함 | 정상 | |
| TC-OR-07 | 확인 화면에 시료명·시료ID·고객명·수량 표시 확인 (FR-O-03) | 시료 1건(S-007, "나노입자") 등록 완료 | 메인2 → S-007 → 홍길동 → 77 → 0(취소) → 0(종료) | "[ 주문 확인 ]", "나노입자", "S-007", "홍길동", "77 ea" 모두 포함 | 정상 | |
| TC-OR-08 | approved_at, released_at 이 null로 저장 확인 | 시료 1건(S-001) 등록 완료 | 메인2 → S-001 → 테스트고객 → 5 → 1(접수) → 0(종료) | orders.json 레코드의 `approved_at.is_null()==true`, `released_at.is_null()==true` | 정상 | |

---

### TG-AP: 주문 처리 — Phase 4 (메뉴 3: 승인·거절·생산 레코드 생성) 검증

#### 사전 조건 공통 사항

- `mvc::App`을 `AppConfig{data_dir = <임시 디렉터리>}`로 초기화한다.
- 각 TC는 별도의 임시 디렉터리를 사용하여 독립적인 초기 상태를 보장한다.
- `std::cin` / `std::cout`을 `std::istringstream` / `std::ostringstream`으로 리디렉션하여 `App::run()`을 구동한다.
- 시료 등록은 메뉴 1을 통해 사전 수행한다. 주문 접수는 메뉴 2를 통해 사전 수행한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-AP-01 | Reserved 주문 없을 때 안내 메시지 출력 | 빈 DB | 메인3 → 0(종료) | "처리 대기 중인 주문이 없습니다" 포함 | 경계 | PASS |
| TC-AP-02 | 재고 충분(shortage=0) → 승인 → Confirmed 전환 확인 | 시료(재고100) + 주문(수량50) | 메인3 → 1(선택) → 1(승인) → 0(종료) | orders.json의 order_status==3(Confirmed), 출력에 "Confirmed" 포함 | 정상 | PASS |
| TC-AP-03 | 재고 부족, Producing 없음(Case 1) → 승인 → Producing 전환 + 생산 레코드 생성 확인 | 시료(재고30) + 주문(수량50) | 메인3 → 1(선택) → 1(승인) → 0(종료) | order_status==2(Producing), productions.json 1건, shortage==20 | 정상 | PASS |
| TC-AP-04 | 재고 부족, Producing 이미 존재(Case 2) → shortage = order_quantity 확인 | 시료(재고30) + 주문1(50) + 주문2(40) | 메인3 → 1 → 1 → 1 → 1 → 0(종료) | 두 번째 생산 레코드의 shortage==40 (order_quantity 전량) | 정상 | PASS |
| TC-AP-05 | 거절 → Rejected 전환 확인 | 시료(재고100) + 주문(수량50) | 메인3 → 1(선택) → 2(거절) → 0(종료) | order_status==1(Rejected), 출력에 "Rejected" 포함 | 정상 | PASS |
| TC-AP-06 | 취소(0) → 주문 상태 미변경 확인 | 시료(재고100) + 주문(수량50) | 메인3 → 1(선택) → 0(취소) → 0(목록) → 0(종료) | order_status==0(Reserved) 유지 | 정상 | PASS |
| TC-AP-07 | approved_at 기록 확인 (승인 시) | 시료(재고100) + 주문(수량50) | 메인3 → 1 → 1(승인) → 0(종료) | approved_at이 null 아님, "YYYY-MM-DD HH:MM:SS" 형식(크기19, 구분자 일치) | 정상 | PASS |
| TC-AP-08 | production_start_at — 큐 빈 경우 = approved_at | 시료(재고10) + 주문(수량50) | 메인3 → 1 → 1(승인 → Producing) → 0(종료) | prod_records[0].production_start_at == ord_records[0].approved_at | 정상 | PASS |
| TC-AP-09 | production_start_at — 선행 Producing 있을 경우 = 선행 완료 시각 | 시료(재고0) + 주문1(50) + 주문2(40) | 메인3 → 주문1 승인 → 주문2 승인 → 0(종료) | prod_records[1].production_start_at == Timestamp::format(completion_epoch(prod_records[0])) | 정상 | PASS |
| TC-AP-10 | 가용 재고 계산: Confirmed 선점분 차감 확인 | 시료(재고100) + 주문1(80) + 주문2(30) | 메인3 → 주문1 승인(Confirmed) → 주문2 승인(Producing) → 0(종료) | 두 번째 주문: shortage==10(=30−20), 출력에 "20 ea" 가용재고 포함 | 정상 | PASS |

---

### TG-MN: 모니터링 — Phase 5 (메뉴 4: 주문량 확인·재고량 확인) 검증

#### 사전 조건 공통 사항

- `mvc::App`을 `AppConfig{data_dir = <임시 디렉터리>}`로 초기화한다.
- 각 TC는 별도의 임시 디렉터리를 사용하여 독립적인 초기 상태를 보장한다.
- `std::cin` / `std::cout`을 `std::istringstream` / `std::ostringstream`으로 리디렉션하여 `App::run()`을 구동한다.
- 시료·주문 픽스처는 `DataStore`를 통해 직접 삽입하거나 메뉴 입력 시퀀스로 생성한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-MN-01 | 주문 없을 때 상태별 건수 모두 0 출력 | 빈 DB | 메인4 → 서브1(주문량 확인) → 0 → 0 | "Reserved  : 0 건", "Producing : 0 건", "Confirmed : 0 건", "Released  : 0 건" 모두 포함 | 경계 | PASS |
| TC-MN-02 | Reserved 2건, Producing 1건, Confirmed 1건, Released 1건, Rejected 1건 → Rejected 제외하고 각 건수 정확히 출력 | 시료 1건 + 각 상태별 주문 직접 삽입 | 메인4 → 서브1 → 0 → 0 | "Reserved  : 2 건", "Producing : 1 건", "Confirmed : 1 건", "Released  : 1 건" 포함. "Rejected  :" 미포함 | 정상 | PASS |
| TC-MN-03 | 시료 없을 때 "등록된 시료가 없습니다." 출력 | 빈 DB | 메인4 → 서브2(재고량 확인) → 0 → 0 | "등록된 시료가 없습니다" 포함 | 경계 | PASS |
| TC-MN-04 | current_stock=0 → 재고 상태 "고갈" | 시료(재고=0) 직접 삽입 | 메인4 → 서브2 → 0 → 0 | 출력에 "고갈" 포함 | 정상 | PASS |
| TC-MN-05 | current_stock>0, Producing 없음, stock >= Reserved합+Confirmed합 → "여유" | 시료(재고=100) + Reserved(30) + Confirmed(20) | 메인4 → 서브2 → 0 → 0 | 출력에 "여유" 포함 | 정상 | PASS |
| TC-MN-06 | current_stock>0, Producing 있음 → "부족" | 시료(재고=100) + Producing(50) | 메인4 → 서브2 → 0 → 0 | 출력에 "부족" 포함 | 정상 | PASS |
| TC-MN-07 | current_stock>0, Producing 없음, stock < Reserved합+Confirmed합 → "부족" | 시료(재고=40) + Reserved(30) + Confirmed(20) | 메인4 → 서브2 → 0 → 0 | 출력에 "부족" 포함 | 정상 | PASS |
| TC-MN-08 | current_stock=0이면서 Producing도 있는 경우 → 우선순위 "고갈" | 시료(재고=0) + Producing(30) + Reserved(10) | 메인4 → 서브2 → 0 → 0 | 출력에 "고갈" 포함 | 경계 | PASS |
| TC-MN-09 | 여러 시료 혼재 시 시료별 독립 판정 확인 | 시료A(재고=0, 고갈) + 시료B(재고=100+Reserved30+Confirmed10, 여유) + 시료C(재고=10+Producing20, 부족) | 메인4 → 서브2 → 0 → 0 | 출력에 "고갈", "여유", "부족" 모두 포함 | 정상 | PASS |
| TC-MN-10 | menu_monitoring() 진입마다 파일 최신 데이터 반영(refresh 호출 여부) 확인 | 1차: 시료 등록 + 주문량 확인(0건). 2차: 주문 접수. 3차: 재진입 | 3차 실행: 메인4 → 서브1 → 0 → 0 | "Reserved  : 1 건" 포함 (파일 갱신 반영) | 정상 | PASS |

---

---

### TG-RL: 출고 처리 — Phase 6 (메뉴 5) 검증

#### 사전 조건 공통 사항

- `mvc::App`을 `AppConfig{data_dir = <임시 디렉터리>}`로 초기화한다.
- 각 TC는 별도의 임시 디렉터리를 사용하여 독립적인 초기 상태를 보장한다.
- `std::cin` / `std::cout`을 `std::istringstream` / `std::ostringstream`으로 리디렉션하여 `App::run()`을 구동한다.
- 시료·주문 픽스처는 `DataStore`를 통해 직접 삽입하거나 메뉴 입력 시퀀스로 생성한다.
- Confirmed 주문 삽입 시 `order_status = 3(Confirmed)`, `approved_at = "YYYY-MM-DD HH:MM:SS"`, `released_at = null`로 설정한다.

---

| TC | 설명 | 사전 조건 | 입력 시퀀스 | 기대 출력 | 분류 | 판정 |
|----|------|----------|------------|----------|------|------|
| TC-RL-01 | Confirmed 주문 없을 때 안내 메시지 출력 후 반환 | 빈 DB (또는 Confirmed 없는 DB) | 메인5 → (자동 false 반환) → 0(종료) | 출력에 "출고 가능한 주문이 없습니다" 포함 | 경계 | |
| TC-RL-02 | 출고 선택 → order_status = Released(4) 전환 확인 | 시료(재고100) + Confirmed 주문(수량50) 직접 삽입 | 메인5 → 1(선택) → 1(출고) → 0(종료) | orders.json 레코드의 `order_status == 4(Released)` | 정상 | |
| TC-RL-03 | 출고 선택 → released_at 기록 확인 (null이 아닌 타임스탬프) | 시료(재고100) + Confirmed 주문(수량50) 직접 삽입 | 메인5 → 1(선택) → 1(출고) → 0(종료) | orders.json 레코드의 `released_at.is_null() == false`, 크기==19, `"YYYY-MM-DD HH:MM:SS"` 형식 | 정상 | |
| TC-RL-04 | 출고 선택 → current_stock -= order_quantity 확인 | 시료(재고100) + Confirmed 주문(수량50) 직접 삽입 | 메인5 → 1(선택) → 1(출고) → 0(종료) | samples.json 레코드의 `current_stock == 50` (100 - 50) | 정상 | |
| TC-RL-05 | 취소(0) 선택 → order_status 미변경, current_stock 미변경 확인 | 시료(재고100) + Confirmed 주문(수량50) 직접 삽입 | 메인5 → 1(선택) → 0(취소) → 0(목록) → 0(종료) | orders.json 레코드의 `order_status == 3(Confirmed)` 유지. samples.json 레코드의 `current_stock == 100` 유지 | 정상 | |
| TC-RL-06 | 결과 출력에 주문번호·출고수량·처리일시·상태변화 포함 확인 (FR-R-06) | 시료(재고100) + Confirmed 주문("ORD-20240501-003", 수량100) 직접 삽입 | 메인5 → 1(선택) → 1(출고) → 0(종료) | 출력에 "출고 처리가 완료되었습니다", "ORD-20240501-003", "100 ea", "Confirmed", "Released" 모두 포함 | 정상 | |
| TC-RL-07 | 출고 후 목록 재조회 시 해당 주문이 Confirmed 목록에서 제거 확인 | 시료(재고100) + Confirmed 주문 1건(수량50) 직접 삽입 | 메인5 → 1(선택) → 1(출고) → (루프 재진입, 자동 false 반환) → 0(종료) | 두 번째 메인5 진입 후 "출고 가능한 주문이 없습니다" 출력 | 정상 | |
| TC-RL-08 | current_stock은 출고 직전 최신 파일 값 기준으로 차감 확인 | 시료(초기재고200, 이후 별도 DataStore로 재고 180으로 update) + Confirmed 주문(수량50) 직접 삽입 | 메인5 → 1(선택) → 1(출고) → 0(종료) | samples.json 레코드의 `current_stock == 130` (180 - 50) — App 초기화 전 재고가 아닌 출고 직전 최신 값 180 기준 | 정상 | |

---

## 합계

| 그룹 | TC 수 |
|------|------|
| TG-DS | 25 |
| TG-DG | 20 |
| TG-MT | 20 |
| TG-MVC | 10 |
| TG-TS | 19 |
| TG-PC | 13 |
| TG-IU | 15 |
| TG-TP | 10 |
| TG-SM | 20 |
| TG-OR | 8 |
| TG-AP | 10 |
| TG-MN | 10 |
| TG-RL | 8 |
| **합계** | **188** |
