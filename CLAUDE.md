# SampleOrderSystem

반도체 시료·주문·생산 데이터를 관리하는 콘솔 애플리케이션.  
C++17, CMake 기반. 빌드 환경은 `BUILD.md` 참조.

---

## 요구사항

기능·비기능 요구사항 전체는 [`docs/PRD.md`](docs/PRD.md) 참조.

---

## 모듈 구성

모듈별 역할·호출 관계는 [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) 참조.

### `lib/json/` → CMake 타겟: `json_lib`
`lib/json/include/json/json.hpp`로 포함. 다른 모든 모듈의 전이 의존성.

### `lib/persistence/` → CMake 타겟: `persistence`
JSON 파일 기반 CRUD (`DataStore`). `json_lib`에 PUBLIC 링크.  
`data_store.hpp`로 포함.

### `src/mvc/` → CMake 타겟: `mvc` (static library)
메인 애플리케이션 셸. 외부 의존성 없음(STL only).  
`#include "mvc/App.hpp"` → `mvc::App().run()` 호출.

### `src/dummy_generator/` → CMake 타겟: `dummy_generator` (static library)
`persistence`에 PRIVATE 링크. `#include "dummy_generator/dummy_generator.hpp"`.

### `src/monitor/` → CMake 타겟: `monitor_lib` (static library)
도메인 타입: `Sample`, `Order`, `Production`.  
`persistence`에 PUBLIC 링크. `#include "ui/app.hpp"` → `App({...}).run()`.

---

## 빌드 (요약)

```bat
:: MSVC Release
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
:: → build\Release\SampleOrderSystem.exe

:: Clang + Ninja (테스트)
cmake -B build-test -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ^
      -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build build-test
```

cmake 경로: `C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe`

---

## 주의 사항

- `monitor_lib`은 **조회 전용**이다. DataStore의 create/update/remove는 노출하지 않는다.
- `dummy_generator`는 시드 42로 고정 → 같은 설정이면 항상 동일한 데이터 출력.
- `lib/json` C4819 경고는 무시해도 된다(코드 페이지 문제, 빌드 결과 무관).
