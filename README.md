# SampleOrderSystem

반도체 시료(Sample)의 생산주문을 접수·승인·생산·출고까지 추적·관리하는 콘솔 기반 생산주문관리 시스템.  
C++17, CMake 기반. Windows 전용.

---

## 환경 종속성

이 프로젝트의 빌드 환경은 **Visual Studio 2026 Community** 설치에 강하게 종속되어 있다.  
아래 툴체인은 모두 VS 2026 설치 경로에서 직접 참조하며, 시스템 PATH 등록 없이 전체 경로로 지정한다.

### 필수 구성요소

| 구성요소 | 버전 | 설치 경로 (기본) |
|---------|------|----------------|
| Visual Studio | 2026 Community | `C:\Program Files\Microsoft Visual Studio\18\Community\` |
| MSVC | 19.50 | VS 설치 포함 |
| Clang (LLVM) | 20.1.8 | VS → "C++용 Clang 컴파일러" 워크로드 |
| CMake | 4.2.3 | VS → "C++ CMake Tools" 워크로드 |
| Ninja | 1.12.1 | VS → CMake Tools 포함 |
| Windows SDK | 10.0.26100.0 | VS 설치 포함 |

> Clang과 Ninja는 VS 워크로드로 설치되므로 별도 설치가 필요 없다.  
> 버전이 다르거나 경로가 다르면 `BUILD.md`의 빌드 명령을 환경에 맞게 수정해야 한다.

---

## 빠른 빌드

상세 빌드 명령 및 커버리지 측정은 [`BUILD.md`](BUILD.md) 참조.

```bat
:: MSVC Release 빌드
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

---

## 문서

| 문서 | 경로 | 설명 |
|------|------|------|
| 요구사항 | [`docs/PRD.md`](docs/PRD.md) | 기능·비기능 요구사항 |
| 아키텍처 | [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | 모듈 역할·의존 구조 |
| 개발 계획 | [`docs/PLAN.md`](docs/PLAN.md) | Phase별 구현 계획 |
| 데이터 스키마 | [`lib/persistence/DATA_SCHEMA.md`](lib/persistence/DATA_SCHEMA.md) | JSON 필드 정의 |
| 빌드 가이드 | [`BUILD.md`](BUILD.md) | 빌드·테스트·커버리지 명령 |
