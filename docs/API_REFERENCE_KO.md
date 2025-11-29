# Integrated Thread System API 레퍼런스

**버전**: 1.0
**최종 수정일**: 2025-11-30
**언어**: [English](API_REFERENCE.md) | [한국어]

---

## 개요

이 문서는 integrated_thread_system의 공개 API에 대한 포괄적인 레퍼런스를 제공합니다.

---

## 목차

1. [핵심 클래스](#핵심-클래스)
2. [설정 API](#설정-api)
3. [스레딩 API](#스레딩-api)
4. [로깅 API](#로깅-api)
5. [모니터링 API](#모니터링-api)

---

## 핵심 클래스

### integrated_system

메인 시스템 클래스로 모든 컴포넌트를 통합 관리합니다.

```cpp
namespace integrated_thread_system {

class integrated_system {
public:
    // 생성
    static auto create() -> std::shared_ptr<integrated_system>;
    static auto create(const config& cfg) -> std::shared_ptr<integrated_system>;

    // 생명주기
    auto start() -> result_void;
    auto stop() -> result_void;
    auto is_running() const -> bool;

    // 컴포넌트 접근
    auto get_thread_pool() -> std::shared_ptr<thread_pool>;
    auto get_logger() -> std::shared_ptr<logger>;
    auto get_monitor() -> std::shared_ptr<monitor>;
};

} // namespace integrated_thread_system
```

---

## 설정 API

### config 구조체

```cpp
struct config {
    // 스레드 풀 설정
    size_t thread_pool_size = std::thread::hardware_concurrency();
    bool enable_work_stealing = true;

    // 로깅 설정
    log_level default_log_level = log_level::info;
    std::string log_file_path;
    bool enable_console_logging = true;

    // 모니터링 설정
    bool enable_monitoring = true;
    bool adaptive_monitoring = true;
    std::chrono::milliseconds collection_interval{1000};
};
```

---

## 스레딩 API

### 작업 제출

```cpp
// 기본 작업 제출
auto future = system->get_thread_pool()->submit([]() {
    return compute_result();
});

// 우선순위 지정 작업
system->get_thread_pool()->submit_with_priority(
    priority::high,
    []() { /* 긴급 작업 */ }
);
```

### 취소 토큰

```cpp
auto token = cancellation_token::create();
system->get_thread_pool()->submit([token]() {
    while (!token->is_cancelled()) {
        // 작업 수행
    }
});

// 나중에 취소
token->cancel();
```

---

## 로깅 API

### 로그 작성

```cpp
auto logger = system->get_logger();

logger->trace("상세 디버그 정보");
logger->debug("디버그 메시지");
logger->info("정보 메시지");
logger->warn("경고 메시지");
logger->error("오류 메시지");
logger->critical("치명적 오류");
```

### 구조화된 로깅

```cpp
logger->info("사용자 로그인", {
    {"user_id", user.id},
    {"ip", request.ip},
    {"timestamp", now()}
});
```

---

## 모니터링 API

### 메트릭 기록

```cpp
auto monitor = system->get_monitor();

// 카운터
monitor->increment("requests_total");

// 게이지
monitor->set_gauge("active_connections", conn_count);

// 히스토그램
monitor->record_histogram("request_latency_ms", latency);
```

### 헬스 체크

```cpp
monitor->register_health_check("database", []() {
    return db->is_connected()
        ? health_status::healthy
        : health_status::unhealthy;
});
```

---

## 관련 문서

- [기능](FEATURES_KO.md)
- [빠른 시작](guides/QUICK_START.md)
- [예제](guides/EXAMPLES.md)
