# Integrated Thread System 변경 이력

이 프로젝트의 모든 주요 변경 사항은 이 파일에 문서화됩니다.

이 형식은 [Keep a Changelog](https://keepachangelog.com/ko/1.0.0/)를 기반으로 하며,
이 프로젝트는 [Semantic Versioning](https://semver.org/lang/ko/)을 준수합니다.

## [Unreleased]

### 추가됨
- 문서 구조 표준화
- 한국어 문서 지원

---

## [1.0.0] - 2025-11-12

### 추가됨
- **통합 시스템**: thread_system, logger_system, monitoring_system 통합
  - 제로 설정 구성
  - 엔터프라이즈급 스레딩 프레임워크
  - 통합 관측성
- **설정 시스템**: 중앙 집중식 설정 관리
  - 적응형 모니터링 지원
  - 스케줄러 설정
  - 향상된 안정성 기능
- **컴포넌트 통합**:
  - thread_system v1.0.0: 락프리 스레드 풀, 우선순위 스케줄링
  - logger_system v1.0.0: 비동기 로깅 (4.34M+ logs/sec)
  - monitoring_system v2.0.0: 적응형 모니터링, 헬스 체크

### 의존성
- common_system v1.0.0: Result<T>, 독립형 이벤트 버스
- thread_system v1.0.0: 스케줄러, 서비스 레지스트리, 크래시 핸들러
- logger_system v1.0.0: 백엔드 아키텍처, 포매터, 보안
- monitoring_system v2.0.0: 적응형 모니터링, 헬스 체크, 안정성

---

## [0.9.0] - 2025-10-15

### 추가됨
- 핵심 서브시스템 초기 통합
- 기본 설정 시스템
- 통합 테스트 프레임워크

### 변경됨
- CMake FetchContent로 의존성 관리 마이그레이션

---

*개별 컴포넌트의 상세 변경 사항은 다음을 참조하세요:*
- [thread_system CHANGELOG](https://github.com/kcenon/thread_system/blob/main/CHANGELOG.md)
- [logger_system CHANGELOG](https://github.com/kcenon/logger_system/blob/main/CHANGELOG.md)
- [monitoring_system CHANGELOG](https://github.com/kcenon/monitoring_system/blob/main/CHANGELOG.md)
