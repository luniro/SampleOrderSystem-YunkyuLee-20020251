# Dummy Data Generation Rules

더미 데이터 생성 규칙 및 공식 정의.  
`DataStore`의 스키마 규약(`DATA_SCHEMA.md`)과 독립적으로, 이 모듈이 값을 어떻게 생성하는지를 기술한다.

---

## 1. Sample

### sample_id
- 형식: `S-{N}` (N은 1부터 시작하는 순번)
- N < 1000: 3자리 0-패딩 → `S-001`, `S-042`, `S-999`
- N ≥ 1000: 패딩 없음 → `S-1000`, `S-12993`

### sample_name
두 가지 요소의 조합 + 사이즈로 구성한다.

```
{소재} {웨이퍼 유형} - {사이즈}
```

| 풀 | 목록 |
|----|------|
| 소재 | 실리콘, 산화막, GaN, SiC, InP, GaAs, 사파이어, 게르마늄 |
| 웨이퍼 유형 | 웨이퍼, 에피텍셜, 파워기판, 기판 |
| 사이즈 | 2인치, 4인치, 6인치, 8인치, 12인치 |

**예시:** `GaN 에피텍셜 - 6인치`, `SiC 파워기판 - 4인치`

### avg_production_time
- 단위: min/ea (개당 생산 소요 시간)
- 범위: 0.1 ~ 2.0
- 분포: 70% 확률로 [0.1, 1.0), 30% 확률로 [1.0, 2.0] (균등분포)
- 유효 자리: 소수점 첫째 자리 (0.1 단위로 반올림)

### yield_rate
- 범위: 0.70 ~ 0.99 (하한 0.70, 사실상 상한 1.0 미만)
- 분포: 정규분포 N(μ=0.90, σ=0.07), 범위 외 값은 경계로 클램프
  - P(yield_rate ≥ 0.90) ≈ 50% (절반 이상이 0.90 이상)
  - P(yield_rate < 0.70) ≈ 0.2% (사실상 발생 안 함)
- 유효 자리: 소수점 둘째 자리 (0.01 단위로 반올림)

### current_stock
- 범위: 50 ~ 500 (균등분포, 단위: 개)

---

## 2. Order

### order_number
- 형식: `ORD-YYYYMMDD-{NNN}` (날짜 고정, N은 3자리 0-패딩 순번)
- 예시: `ORD-20240501-001`

### sample_id
- 생성된 Sample 목록에서 무작위 선택 → 참조 일관성 보장

### order_quantity
- 범위: 10 ~ 500 (균등분포, 단위: 개)
- 수십~수백 개 규모를 반영

### order_status
| 값 | 의미 |
|----|------|
| `0` | Reserved |
| `1` | Rejected |
| `2` | Producing |
| `3` | Confirmed |
| `4` | Release |

---

## 3. Production

### order_number / sample_name / order_quantity
- 생성된 Order 목록에서 무작위 선택 후 해당 Order의 값을 그대로 사용
- `sample_name`은 선택된 Order가 참조하는 Sample의 이름으로 결정 → 참조 일관성 보장

### shortage
- 범위: 0 ~ order_quantity (균등분포, 단위: 개)
- 0이면 재고 충분, 양수이면 추가 생산 필요

### actual_production
부족분(shortage)을 실제 수율로 커버하기 위한 생산량:

```
actual_production = ceil(shortage / (yield_rate × 0.9))
```

- `yield_rate`: 해당 Order가 참조하는 Sample의 수율
- `0.9` 계수: 공정 효율 보정 (수율의 90%만 실질 유효하다고 가정)
- shortage = 0이면 actual_production = 0

**예시:**
```
shortage = 50, yield_rate = 0.85
→ actual_production = ceil(50 / (0.85 × 0.9)) = ceil(50 / 0.765) = ceil(65.36) = 66
```

### ordered_at
- 2024-01-01T00:00:00 UTC 기준으로 0~180일 범위의 무작위 오프셋 적용
- 형식: `HH:MM` (시각만 저장, 날짜 정보 미포함)

### estimated_completion
총 생산 소요 시간(duration):

```
estimated_completion = actual_production (ea) × avg_production_time (min/ea)
```

- 절대 시각이 아닌 **소요 시간**을 나타냄
- actual_production = 0이면 `"00:00"` (생산 불필요)
- 형식: `HH:MM` (시간:분, duration)

**예시:**
```
actual_production = 66, avg_production_time = 0.5 min/ea
→ total = 33.0분 → "00:33"

actual_production = 338, avg_production_time = 1.6 min/ea
→ total = 540.8분 → "09:01"
```

---

## 난수 생성

- 엔진: `std::mt19937`, 시드 고정 = `42` (재현 가능한 데이터)
- 동일 시드로 실행 시 항상 동일한 결과를 생성함
