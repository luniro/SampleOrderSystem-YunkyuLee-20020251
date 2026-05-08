# dummy_generator

반도체 시료·주문·생산 레코드의 더미 데이터를 JSON 파일로 생성하는 모듈.  
폴더 단위로 복사하여 다른 CMake 프로젝트에 이식할 수 있도록 설계되었다.

---

## 의존성

| 라이브러리 | 역할 | 비고 |
|------------|------|------|
| `persistence` | JSON 파일 기반 CRUD (`DataStore`) | `lib/persistence/` |
| `json_lib` | JSON 파싱·직렬화 | `persistence`가 내부적으로 의존 |

`dummy_generator`는 `persistence`만 직접 참조한다. `json_lib`는 전이 의존성으로 별도 링크 불필요.

---

## CMake 통합

```cmake
# 의존 라이브러리가 먼저 선언되어 있어야 한다
add_subdirectory(lib/json)
add_subdirectory(lib/persistence)

# 모듈 추가
add_subdirectory(path/to/dummy_generator)

# 실행파일 또는 라이브러리에 링크
target_link_libraries(your_target PRIVATE dummy_generator)
```

`dummy_generator`의 PUBLIC include 경로는 자신의 **부모 디렉터리**로 설정되므로,  
헤더는 `"dummy_generator/dummy_generator.hpp"` 형태로 포함한다.

```cpp
#include "dummy_generator/dummy_generator.hpp"
```

---

## API

### GeneratorConfig

```cpp
struct GeneratorConfig {
    int         sample_count     = 5;   // 생성할 Sample 수
    int         order_count      = 10;  // 생성할 Order 수
    int         production_count = 8;   // 생성할 Production 수
    std::string output_dir       = "."; // JSON 파일 출력 디렉터리
};
```

### generate_dummy_data()

```cpp
void generate_dummy_data(const GeneratorConfig& config);
```

- `output_dir`이 없으면 자동 생성
- `sample_count`, `order_count`, `production_count` 중 하나라도 0 이하면 `std::invalid_argument` throw
- 항상 동일한 시드(42)로 실행 → **같은 설정이면 항상 같은 데이터 생성**

**출력 파일:**

| 파일 | 내용 |
|------|------|
| `samples.json` | Sample 레코드 |
| `orders.json` | Order 레코드 (Sample 참조 일관성 보장) |
| `productions.json` | Production 레코드 (Order·Sample 참조 일관성 보장) |

---

## 사용 예시

```cpp
GeneratorConfig config;
config.sample_count     = 10;
config.order_count      = 30;
config.production_count = 20;
config.output_dir       = "./data";

generate_dummy_data(config);
// → ./data/samples.json, orders.json, productions.json 생성
```

---

## 생성 데이터 규칙

값 분포, ID 형식, 공식 등 세부 생성 규칙은 [`GENERATION_RULES.md`](GENERATION_RULES.md) 참조.  
스키마(필드 정의)는 `lib/persistence/DATA_SCHEMA.md` 참조.
