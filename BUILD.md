# Build Guide

## 툴체인

### 일반 빌드 (MSVC)
| 항목 | 버전 |
|------|------|
| 컴파일러 | MSVC 19.50.35730.0 (`cl.exe`) |
| IDE / 빌드 도구 | Visual Studio 2026 Community (msbuild 18.5.4) |
| CMake | VS 내장 CMake (`Visual Studio 18 2026` 제너레이터 사용) |
| Windows SDK | 10.0.26100.0 |
| 아키텍처 | x64 |
| C++ 표준 | C++17 |

### 테스트 빌드 (Clang)
| 항목 | 버전 |
|------|------|
| 컴파일러 | Clang 20.1.8 (`clang.exe` / VS 2026 LLVM 내장) |
| 빌드 시스템 | CMake 4.2.3 + Ninja 1.12.1 |
| 테스트 프레임워크 | gtest / gmock |
| 커버리지 도구 | llvm-cov / llvm-profdata |
| C++ 표준 | C++17 |

---

## cmake 경로

시스템 PATH에 cmake가 등록되어 있지 않으므로 VS 내장 cmake를 사용한다.

```
C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
```

이하 빌드 명령에서 `cmake`는 위 전체 경로를 가리킨다.

---

## 빌드 출력 디렉터리

| 디렉터리 | 용도 |
|---------|------|
| `build/` | MSVC 빌드 출력 (`build\Release\SampleOrderSystem.exe`) |
| `build-test/` | Clang+Ninja 테스트 빌드 출력 |

> 모듈 구조·디렉터리 레이아웃은 [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) 참조.

---

## CMake 옵션

| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `ENABLE_TESTS` | `OFF` | 테스트 빌드 활성화 (Clang 커버리지 플래그 + gtest 포함) |
| `JSON_BUILD_TESTS` | `OFF` | json 라이브러리 내부 테스트 빌드 (강제 비활성화) |

---

## 빌드 방법

### 일반 빌드 (MSVC)

```bat
:: configure
cmake -B build -G "Visual Studio 18 2026" -A x64

:: build (Release)
cmake --build build --config Release

:: build (Debug)
cmake --build build --config Debug
```

출력물: `build\Release\SampleOrderSystem.exe`

> **알려진 경고**: `lib/json` 소스 파일에서 C4819 경고(코드 페이지 문자 관련)가 출력되나 빌드 결과에는 영향 없음.

---

### 테스트 빌드 (Clang + Ninja)

`build/`는 MSVC 제너레이터 캐시가 있으므로 반드시 `build-test/`를 별도로 사용한다.  
Clang이 PATH에 등록되지 않으므로 전체 경로를 지정한다.

```bat
:: configure
cmake -S . -B build-test -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON ^
  -DCMAKE_CXX_COMPILER="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang++.exe" ^
  -DCMAKE_C_COMPILER="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang.exe" ^
  -DCMAKE_MAKE_PROGRAM="C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe" ^
  -DCMAKE_RC_COMPILER="C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\rc.exe"

:: build
cmake --build build-test
```

---

### 커버리지 측정 (테스트 빌드 후)

`<target>`은 CMakeLists.txt에서 확인한 테스트 타깃명으로 대체한다.

```bat
:: 프로파일 데이터 병합
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\llvm-profdata.exe" ^
  merge -sparse build-test\<target>.profraw -o build-test\<target>.profdata

:: 커버리지 리포트 출력
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\llvm-cov.exe" ^
  report build-test\<target>.exe ^
  -instr-profile=build-test\<target>.profdata ^
  -ignore-filename-regex="(googletest|gtest|gmock|tests/)"

:: HTML 리포트 생성
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\llvm-cov.exe" ^
  show build-test\<target>.exe ^
  -instr-profile=build-test\<target>.profdata ^
  -format=html -output-dir=coverage-report
```

---

## 컴파일 옵션 요약

| 구분 | 플래그 |
|------|--------|
| MSVC 경고 | `/W4 /utf-8` |
| Clang 경고 | `-Wall -Wextra` |
| Clang 커버리지 | `-fprofile-instr-generate -fcoverage-mapping` |
