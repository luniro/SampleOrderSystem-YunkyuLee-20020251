# SampleOrderSystem — Architecture

> **참조**: [CLAUDE.md](../CLAUDE.md) (빌드·CMake 구조) · [DATA_SCHEMA.md](../lib/persistence/DATA_SCHEMA.md) (데이터 스키마)

---

## 1. 모듈 역할

| 모듈 | 역할 | 비고 |
|------|------|------|
| `src/mvc` | **메인 진입점**. MVC 구조의 콘솔 애플리케이션 셸. 메뉴 루프·화면 전환·비즈니스 로직 제어 담당. Controller가 백그라운드 상태 전환(FR-L-09)을 포함한 전체 흐름을 관장한다. | App::run() 호출 |
| `src/monitor` | 데이터 **조회·표 렌더링** 전용 어댑터. 시료·주문·생산 현황을 표 형태로 출력. 표 UI 변경 시 이 모듈을 수정한다. | 읽기 전용 |
| `lib/persistence` | JSON 파일 기반 **CRUD**. 시료·주문·생산 데이터의 생성·수정·삭제 담당. | DataStore |
| `src/dummy_generator` | **기능 테스트용** 더미 데이터 생성. 시드 42 고정으로 재현 가능. | 개발·테스트 전용 |
| `lib/json` | JSON 파싱·직렬화. persistence의 내부 의존성. | 자체 구현 |

---

## 2. 모듈 간 호출 관계

```
[ 사용자 ]
    │
    ▼
 mvc (App::run)          ← 메인 진입점, 메뉴 루프
    ├── monitor          ← 조회 화면 렌더링
    └── persistence      ← 데이터 CRUD (시료·주문·생산)

 dummy_generator         ← 독립 실행 (테스트 데이터 초기화 시 별도 호출)
    └── persistence
```

---

## 3. CMake 의존 그래프

```
SampleOrderSystem (exe)
 ├── mvc              (STL only)
 ├── dummy_generator  → persistence → json_lib
 └── monitor_lib      → persistence → json_lib
```

> CMake 타겟별 include 경로·링크 방식은 [CLAUDE.md](../CLAUDE.md) 참조.
