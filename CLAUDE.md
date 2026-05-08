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

## 개발 프로세스

각 Phase는 아래 순서로 진행한다. **커밋은 반드시 사람이 확인한 후 수행한다.**

```
1. 상세 설계   docs/design/phase{N}.md 작성
                  ↓
2. 구현 루프   orchestrator 에이전트 실행
               (doc-verifier → implementer + sheet-writer → compliance-verifier → test-verifier)
                  ↓
3. 사람 확인   바이너리 동작·코드·테스트 결과 직접 검토
                  ↓
4. 커밋        Phase 1개 = 커밋 1개 (사람 승인 후에만 수행)
```

- `docs/design/phase{N}.md`는 구현 전 반드시 작성한다. orchestrator와 implementer의 동작 기준이 된다.
- 루프 중 자동 커밋은 수행하지 않는다. 커밋 권한은 사람에게만 있다.

---

## 주의 사항

- `monitor_lib`은 **조회 전용**이다. DataStore의 create/update/remove는 노출하지 않는다.
- `dummy_generator`는 시드 42로 고정 → 같은 설정이면 항상 동일한 데이터 출력.
- `lib/json` C4819 경고는 무시해도 된다(코드 페이지 문제, 빌드 결과 무관).
