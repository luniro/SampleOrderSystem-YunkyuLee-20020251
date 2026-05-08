# Data Schema

이 프로젝트에서 관리하는 세 가지 레코드 타입의 필드 정의.  
`DataStore`는 자유 형식 JSON 오브젝트를 저장하므로, 스키마는 코드 레벨에서 강제되지 않고 이 문서를 규약으로 삼는다.

---

## 1. Sample (시료)

**파일:** `samples.json`

| 필드 | 변수명 | JSON 타입 | 설명 |
|------|--------|-----------|------|
| 시료 ID | `sample_id` | `String` | 시료 고유 식별자 (사용자 지정) |
| 시료명 | `sample_name` | `String` | 시료 이름 |
| 평균 생산시간 | `avg_production_time` | `Float` | 단위: 시간(h) |
| 수율 | `yield_rate` | `Float` | 범위: 0.0 ~ 1.0 (예: 0.85 = 85%) |
| 현재 재고 | `current_stock` | `Integer` | 단위: 개 |

> `id` 필드는 `DataStore`가 자동 부여하는 내부 관리 키로, `sample_id`와 별개다.

**예시 레코드:**
```json
{
    "id": 1,
    "sample_id": "SMP-001",
    "sample_name": "산화철 나노입자",
    "avg_production_time": 4.5,
    "yield_rate": 0.87,
    "current_stock": 200
}
```

---

## 2. Order (주문)

**파일:** `orders.json`

| 필드 | 변수명 | JSON 타입 | 설명 |
|------|--------|-----------|------|
| 주문번호 | `order_number` | `String` | 주문 고유 식별자 (사용자 지정) |
| 시료 ID | `sample_id` | `String` | 주문 대상 시료 (`Sample.sample_id` 참조) |
| 고객명 | `customer_name` | `String` | 주문 고객 이름 |
| 주문수량 | `order_quantity` | `Integer` | 단위: 개 |
| 주문상태 | `order_status` | `Integer` | 아래 상태 코드 참조 |
| 승인 시각 | `approved_at` | `String` | 주문 승인(`Producing` 전환) 시각 (형식: `"YYYY-MM-DD HH:MM:SS"`). 생산 큐 정렬 기준. `Reserved`·`Rejected` 상태에서는 `null`. |
| 출고 처리 시각 | `released_at` | `String` | 출고 처리(`Released` 전환) 시각 (형식: `"YYYY-MM-DD HH:MM:SS"`). `Released` 이외 상태에서는 `null`. |

### 주문상태 코드

| 값 | 의미 |
|----|------|
| `0` | Reserved |
| `1` | Rejected |
| `2` | Producing |
| `3` | Confirmed |
| `4` | Released |

**예시 레코드:**
```json
{
    "id": 1,
    "order_number": "ORD-20240501-001",
    "sample_id": "SMP-001",
    "customer_name": "홍길동",
    "order_quantity": 50,
    "order_status": 2,
    "approved_at": "2024-05-01 10:30:00",
    "released_at": null
}
```

---

## 3. Production (생산)

**파일:** `productions.json`

| 필드 | 변수명 | JSON 타입 | 설명 |
|------|--------|-----------|------|
| 주문번호 | `order_number` | `String` | 연결된 주문 (`Order.order_number` 참조) |
| 시료명 | `sample_name` | `String` | 생산 대상 시료명 (`Sample.sample_name` 참조) |
| 주문수량 | `order_quantity` | `Integer` | 단위: 개 |
| 부족분 | `shortage` | `Integer` | 재고 부족으로 추가 생산이 필요한 수량 (단위: 개) |
| 실제생산량 | `actual_production` | `Integer` | 수율 적용 후 실제 생산해야 하는 수량 (단위: 개). `ceil(shortage / yield_rate)` |
| 주문시각 | `ordered_at` | `String` | 주문 접수 시각 (형식: `"YYYY-MM-DD HH:MM:SS"`) |
| 예상완료 | `estimated_completion` | `String` | 총 생산 소요 시간·duration (형식: `"HH:MM"`, 예: `"03:30"`) |
| 생산 시작 시각 | `production_start_at` | `String` | 이 주문의 생산 시작 예정 시각 (형식: `"YYYY-MM-DD HH:MM:SS"`). enqueue 시점에 확정. 큐가 비어있으면 `approved_at`, 선행 주문이 있으면 큐 마지막 주문의 `production_start_at + estimated_completion`으로 결정. |

**예시 레코드:**
```json
{
    "id": 1,
    "order_number": "ORD-20240501-001",
    "sample_name": "산화철 나노입자",
    "order_quantity": 50,
    "shortage": 20,
    "actual_production": 23,
    "ordered_at": "2024-05-01 10:30:00",
    "estimated_completion": "03:30",
    "production_start_at": "2024-05-01 10:30:00"
}
```

---

## 컬렉션 분리

세 타입은 별도의 JSON 파일로 관리하며, 각각 독립된 `DataStore` 인스턴스를 사용한다.

```cpp
DataStore sample_store("samples.json");
DataStore order_store("orders.json");
DataStore production_store("productions.json");
```

### 논리적 참조 관계

```
Sample ──(sample_id)──► Order ──(order_number)──► Production
         (sample_name)                (order_number)
                                      (sample_name)
                                      (order_quantity)
```

`DataStore`는 외래 키 무결성을 강제하지 않으며, 참조 일관성은 애플리케이션 레벨에서 보장한다.
