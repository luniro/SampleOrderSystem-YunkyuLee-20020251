# persistence

JSON 파일 기반 CRUD 데이터 저장소 모듈. `lib/json`을 백엔드로 사용하며, 이 디렉터리 전체를 다른 프로젝트에 이식할 수 있도록 설계되었다.

---

## 폴더 구조

```
persistence/
├── CMakeLists.txt      # persistence 정적 라이브러리 정의
├── data_store.hpp      # DataStore / RecordNotFoundError 인터페이스
├── data_store.cpp      # CRUD 구현
├── DATA_SCHEMA.md      # 컬렉션별 필드 정의 (프로젝트별로 교체)
└── README.md
```

---

## 이식 방법

### 1. 파일 복사

```
persistence/        →  <대상 프로젝트>/path/to/persistence/
lib/json/           →  <대상 프로젝트>/lib/json/
```

### 2. CMake 통합

대상 프로젝트의 `CMakeLists.txt`에 아래 두 줄을 추가한다.

```cmake
add_subdirectory(lib/json)
add_subdirectory(path/to/persistence)
```

실행 파일에 링크한다. `json_lib` 의존성은 `persistence`가 PUBLIC으로 전파하므로 별도 명시가 불필요하다.

```cmake
target_link_libraries(<target> PRIVATE persistence)
```

### 3. 헤더 포함

```cpp
#include "data_store.hpp"
```

---

## DataStore API

```cpp
// 생성자 — file_path: 데이터를 영속화할 JSON 파일 경로
explicit DataStore(const std::string& file_path);

// Create: 레코드 추가. id는 자동 부여. 부여된 id 반환.
int64_t create(JsonValue record);

// Read (단건): id로 조회. 없으면 RecordNotFoundError.
JsonValue read(int64_t id) const;

// Read (전체): 저장된 모든 레코드 반환 (id 오름차순).
std::vector<JsonValue> read_all() const;

// Update: id 레코드에 fields의 키-값을 병합. 없으면 RecordNotFoundError.
void update(int64_t id, const JsonValue& fields);

// Delete: id 레코드 삭제. 없으면 RecordNotFoundError.
void remove(int64_t id);
```

### 예외 타입

| 예외 | 발생 시점 |
|------|----------|
| `RecordNotFoundError` | 존재하지 않는 id로 read / update / remove 호출 |
| `JsonIOError` | 파일 열기 / 쓰기 실패 |
| `JsonParseError` | 저장된 JSON 파일 파싱 실패 |

### 사용 예시

```cpp
#include "data_store.hpp"

DataStore store("items.json");

// Create
JsonValue rec = JsonValue::object();
rec["name"] = JsonValue(std::string("alice"));
rec["score"] = JsonValue(int64_t(95));
int64_t id = store.create(std::move(rec));

// Read
JsonValue item = store.read(id);

// Update (partial)
JsonValue patch = JsonValue::object();
patch["score"] = JsonValue(int64_t(100));
store.update(id, patch);

// List all
for (const auto& r : store.read_all()) {
    // ...
}

// Delete
store.remove(id);
```

---

## 데이터 스키마

이 모듈이 저장하는 컬렉션별 필드 정의는 [`DATA_SCHEMA.md`](DATA_SCHEMA.md)를 참조한다.  
이식 시 해당 파일을 대상 프로젝트의 스키마에 맞게 교체하면 된다.
