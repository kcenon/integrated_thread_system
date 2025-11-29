# Integrated Thread System 프로젝트 구조

**버전**: 1.0
**최종 수정일**: 2025-11-30
**언어**: [English](PROJECT_STRUCTURE.md) | [한국어]

---

## 개요

이 문서는 integrated_thread_system 저장소의 디렉토리 구조와 파일 구성에 대해 설명합니다.

---

## 디렉토리 구조

```
integrated_thread_system/
├── .github/                    # GitHub 설정
│   └── workflows/              # CI/CD 워크플로우
│       ├── ci.yml
│       ├── coverage.yml
│       └── static-analysis.yml
│
├── cmake/                      # CMake 모듈
│   ├── dependencies.cmake      # 의존성 관리
│   └── options.cmake           # 빌드 옵션
│
├── docs/                       # 문서
│   ├── README.md               # 문서 인덱스
│   ├── API_REFERENCE.md        # API 레퍼런스
│   ├── ARCHITECTURE.md         # 아키텍처 설계
│   ├── BENCHMARKS.md           # 성능 벤치마크
│   ├── CHANGELOG.md            # 변경 이력
│   ├── FEATURES.md             # 기능 설명
│   ├── PRODUCTION_QUALITY.md   # 품질 문서
│   ├── PROJECT_STRUCTURE.md    # 프로젝트 구조 (이 파일)
│   ├── advanced/               # 고급 주제
│   ├── contributing/           # 기여 가이드
│   ├── guides/                 # 사용자 가이드
│   ├── integration/            # 통합 가이드
│   └── performance/            # 성능 문서
│
├── include/                    # 공개 헤더
│   └── integrated_thread_system/
│       ├── integrated_system.h
│       ├── config.h
│       └── exports.h
│
├── src/                        # 소스 코드
│   ├── core/                   # 핵심 구현
│   ├── integration/            # 통합 레이어
│   └── config/                 # 설정 처리
│
├── tests/                      # 테스트
│   ├── unit/                   # 단위 테스트
│   ├── integration/            # 통합 테스트
│   └── utils/                  # 테스트 유틸리티
│
├── examples/                   # 예제 코드
│   ├── basic/                  # 기본 사용법
│   └── advanced/               # 고급 사용법
│
├── benchmarks/                 # 벤치마크
│   └── performance/            # 성능 측정
│
├── CMakeLists.txt              # 메인 CMake 파일
├── README.md                   # 프로젝트 README
├── LICENSE                     # 라이선스
└── CHANGELOG.md                # 변경 이력
```

---

## 주요 디렉토리 설명

### include/

공개 API 헤더 파일들이 위치합니다. 사용자가 포함해야 하는 헤더입니다.

### src/

라이브러리 구현 코드입니다. 내부 구현 세부사항을 포함합니다.

### docs/

프로젝트 문서입니다. 영어와 한국어 버전을 모두 제공합니다.

### tests/

테스트 코드입니다. 단위 테스트, 통합 테스트, 스트레스 테스트를 포함합니다.

### examples/

사용 예제입니다. 기본 사용법부터 고급 패턴까지 다룹니다.

---

## 빌드 아티팩트

빌드 후 생성되는 디렉토리:

```
build/                          # 빌드 출력
├── lib/                        # 라이브러리
├── bin/                        # 실행 파일
├── tests/                      # 테스트 실행 파일
└── _deps/                      # FetchContent 의존성
```

---

## 관련 문서

- [아키텍처](ARCHITECTURE_KO.md)
- [빌드 가이드](guides/BUILD_GUIDE.md)
- [기여하기](contributing/CONTRIBUTING.md)
