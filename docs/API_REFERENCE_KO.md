# Integrated Thread System API 레퍼런스

**버전**: 2.1
**최종 수정일**: 2025-12-10
**언어**: [English](API_REFERENCE.md) | [한국어]

---

## 개요

이 문서는 integrated_thread_system의 공개 API에 대한 포괄적인 레퍼런스를 제공합니다.

---

## 목차

1. [C++20 Concepts 지원](#c20-concepts-지원)
2. [핵심 클래스](#핵심-클래스)
3. [설정](#설정)
4. [작업 제출](#작업-제출)
5. [고급 기능](#고급-기능)
6. [모니터링 및 메트릭](#모니터링-및-메트릭)
7. [유틸리티 타입](#유틸리티-타입)

---

## C++20 Concepts 지원

v2.1.0부터 `integrated_thread_system`은 컴파일 타임 타입 검증과 명확한 에러 메시지를 위해 C++20 Concepts를 사용합니다.

### `std::invocable` 제약

모든 작업 제출 함수는 컴파일 타임에 호출 가능 타입을 검증하기 위해 `std::invocable`을 사용합니다:

```cpp
template<typename F, typename... Args>
    requires std::invocable<F, Args...>
auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
```

**장점:**
- **컴파일 타임 검증**: 잘못된 호출 가능 타입이 런타임이 아닌 컴파일 시 감지됩니다
- **명확한 에러 메시지**: 깊은 템플릿 인스턴스화 오류 대신 명확한 제약 위반 메시지를 받습니다
- **IDE 지원**: 최신 IDE가 코드를 더 잘 이해하고 검증할 수 있습니다

### `VoidCallable` Concept

반복 작업에 사용되는 void 반환 호출 가능 타입을 위한 커스텀 Concept입니다:

```cpp
template<typename F>
concept VoidCallable = std::invocable<F> && std::is_void_v<std::invoke_result_t<F>>;
```

**사용 예시:**
```cpp
// 유효 - void 반환
system.schedule_recurring(1000ms, []() { update_stats(); });

// 무효 - int 반환 (컴파일 에러)
system.schedule_recurring(1000ms, []() { return 42; });
// error: constraints not satisfied for 'schedule_recurring'
```

---

## 핵심 클래스

### `unified_thread_system`

스레딩, 로깅, 모니터링 기능에 대한 통합 접근을 제공하는 메인 클래스입니다.

```cpp
namespace kcenon::integrated {
    class unified_thread_system;
}
```

#### 생성자

```cpp
explicit unified_thread_system(const config& cfg = config());
```
지정된 설정으로 통합 스레드 시스템을 생성합니다. 설정을 제공하지 않으면 자동 감지가 포함된 기본 설정을 사용합니다.

#### 소멸자

```cpp
~unified_thread_system();
```
모든 서브시스템의 graceful shutdown을 자동으로 처리합니다.

---

## 설정

### `config`

시스템 동작을 커스터마이즈하기 위한 설정 구조체입니다.

```cpp
struct config {
    // 기본 설정
    std::string name = "ThreadSystem";
    size_t thread_count = 0;  // 0 = 자동 감지

    // 로깅 설정
    bool enable_file_logging = true;
    bool enable_console_logging = true;
    std::string log_directory = "./logs";
    log_level min_log_level = log_level::info;

    // 고급 기능 (선택적)
    bool enable_circuit_breaker = false;
    size_t circuit_breaker_failure_threshold = 5;
    std::chrono::milliseconds circuit_breaker_reset_timeout{5000};
    size_t max_queue_size = 10000;
    bool enable_work_stealing = true;
    bool enable_dynamic_scaling = false;
    size_t min_threads = 1;
    size_t max_threads = 0;  // 0 = 제한 없음

    // 빌더 패턴 메서드
    config& set_name(const std::string& n);
    config& set_worker_count(size_t c);
    config& set_logging(bool file, bool console);
};
```

---

## 작업 제출

### 기본 작업 제출

#### `submit`
```cpp
template<typename F, typename... Args>
    requires std::invocable<F, Args...>
auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
```
비동기 실행을 위해 작업을 제출합니다. C++20 `std::invocable` concept으로 컴파일 타임 검증을 수행합니다.

**파라미터:**
- `f`: 실행할 함수 또는 호출 가능 객체
- `args`: 함수에 전달할 인자

**반환:** 결과를 포함하는 `std::future`

**예시:**
```cpp
auto future = system.submit([]() { return 42; });
int result = future.get();  // 42
```

### 배치 처리

#### `submit_batch`
```cpp
template<typename Iterator, typename F>
    requires std::invocable<F, typename std::iterator_traits<Iterator>::value_type>
auto submit_batch(Iterator first, Iterator last, F&& func)
    -> std::vector<std::future<std::invoke_result_t<F, typename std::iterator_traits<Iterator>::value_type>>>;
```
여러 항목을 병렬로 처리합니다. C++20 concepts를 사용하여 함수가 이터레이터의 값 타입으로 호출 가능한지 검증합니다.

**파라미터:**
- `first`, `last`: 처리할 항목의 이터레이터 범위
- `func`: 각 항목에 적용할 함수

**반환:** 결과를 포함하는 future 벡터

**예시:**
```cpp
std::vector<int> data = {1, 2, 3, 4, 5};
auto futures = system.submit_batch(data.begin(), data.end(),
    [](int n) { return n * n; });
```

---

## 고급 기능

### 우선순위 기반 제출

#### `submit_with_priority`
```cpp
template<typename F, typename... Args>
    requires std::invocable<F, Args...>
auto submit_with_priority(priority_level priority, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
지정된 우선순위로 작업을 제출합니다.

#### `submit_critical`
```cpp
template<typename F, typename... Args>
    requires std::invocable<F, Args...>
auto submit_critical(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
높은 우선순위 작업을 제출합니다.

#### `submit_background`
```cpp
template<typename F, typename... Args>
    requires std::invocable<F, Args...>
auto submit_background(F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
낮은 우선순위 백그라운드 작업을 제출합니다.

### 취소 지원

#### `create_cancellation_token`
```cpp
std::shared_ptr<void> create_cancellation_token();
```
작업 실행을 제어하기 위한 새 취소 토큰을 생성합니다.

#### `cancel_token`
```cpp
void cancel_token(std::shared_ptr<void> token);
```
이전에 생성한 토큰을 취소하여 연관된 모든 작업에 중지 신호를 보냅니다.

#### `submit_cancellable`
```cpp
template<typename F, typename... Args>
    requires std::invocable<F, Args...>
auto submit_cancellable(std::shared_ptr<void> token, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
제공된 토큰을 통해 취소할 수 있는 작업을 제출합니다.

**예시:**
```cpp
// 취소 토큰 생성
auto token = system.create_cancellation_token();

auto future = system.submit_cancellable(token, []() {
    // 장시간 실행 작업
    return process_data();
});

// 필요시 취소
system.cancel_token(token);
```

### 예약 실행

#### `schedule`
```cpp
template<typename F, typename... Args>
    requires std::invocable<F, Args...>
auto schedule(std::chrono::milliseconds delay, F&& f, Args&&... args)
    -> std::future<std::invoke_result_t<F, Args...>>;
```
지연 후 실행되도록 작업을 예약합니다.

#### `schedule_recurring`
```cpp
template<VoidCallable F>
size_t schedule_recurring(std::chrono::milliseconds interval, F&& f);
```
일정 간격으로 반복 실행되도록 작업을 예약합니다. 커스텀 `VoidCallable` concept을 사용하여 호출 가능 타입이 void를 반환하는지 확인합니다.

**반환:** 취소를 위한 작업 ID

#### `cancel_recurring`
```cpp
void cancel_recurring(size_t task_id);
```
반복 작업을 취소합니다.

### Map-Reduce 패턴

#### `map_reduce`
```cpp
template<typename Iterator, typename MapFunc, typename ReduceFunc, typename T>
auto map_reduce(Iterator first, Iterator last,
                MapFunc&& map_func, ReduceFunc&& reduce_func, T initial)
    -> std::future<T>;
```
병렬 map-reduce 연산을 수행합니다.

**예시:**
```cpp
std::vector<int> numbers = {1, 2, 3, 4, 5};
auto sum_of_squares = system.map_reduce(
    numbers.begin(), numbers.end(),
    [](int n) { return n * n; },      // Map: 제곱
    [](int a, int b) { return a + b; }, // Reduce: 합
    0                                    // 초기값
);
```

---

## 모니터링 및 메트릭

### 성능 메트릭

#### `get_metrics`
```cpp
performance_metrics get_metrics() const;
```
현재 성능 메트릭을 반환합니다.

```cpp
struct performance_metrics {
    size_t tasks_submitted;
    size_t tasks_completed;
    size_t tasks_failed;
    size_t tasks_cancelled;

    std::chrono::nanoseconds average_latency;
    std::chrono::nanoseconds min_latency;
    std::chrono::nanoseconds max_latency;
    std::chrono::nanoseconds p95_latency;
    std::chrono::nanoseconds p99_latency;

    size_t active_workers;
    size_t queue_size;
    double queue_utilization_percent;
    double tasks_per_second;
};
```

### 헬스 상태

#### `get_health`
```cpp
health_status get_health() const;
```
시스템 헬스 정보를 반환합니다.

```cpp
struct health_status {
    health_level overall_health;
    double cpu_usage_percent;
    double memory_usage_percent;
    double queue_utilization_percent;
    bool circuit_breaker_open;
    size_t consecutive_failures;
    std::vector<std::string> issues;
};
```

### 시스템 제어

#### `wait_for_completion`
```cpp
void wait_for_completion();
```
현재 대기 중인 모든 작업이 완료될 때까지 블록합니다.

#### `shutdown`
```cpp
void shutdown();
```
시스템을 gracefully 종료합니다.

#### `shutdown_immediate`
```cpp
void shutdown_immediate();
```
대기 중인 작업을 취소하고 즉시 종료합니다.

### 내보내기 함수

#### `export_metrics_json`
```cpp
std::string export_metrics_json() const;
```
메트릭을 JSON 형식으로 내보냅니다.

#### `export_metrics_prometheus`
```cpp
std::string export_metrics_prometheus() const;
```
메트릭을 Prometheus 형식으로 내보냅니다.

---

## 유틸리티 타입

### `priority_level`
```cpp
enum class priority_level {
    lowest = 0,
    low = 25,
    normal = 50,
    high = 75,
    highest = 100,
    critical = 127
};
```

### `health_level`
```cpp
enum class health_level {
    healthy,
    degraded,
    critical,
    failed
};
```

### `log_level`
```cpp
enum class log_level {
    trace,
    debug,
    info,
    warning,
    error,
    critical,
    fatal
};
```

### `cancellation_token`
```cpp
class cancellation_token {
public:
    cancellation_token();
    void cancel();
    bool is_cancelled() const;
};
```

---

## 에러 처리

모든 메서드는 강력한 예외 안전 보장을 제공합니다. 예외를 throw하는 작업은 future의 `get()` 메서드를 통해 예외를 전파합니다.

```cpp
try {
    auto future = system.submit([]() {
        throw std::runtime_error("작업 실패");
    });
    future.get();  // 예외 throw
} catch (const std::exception& e) {
    std::cerr << "작업 오류: " << e.what() << std::endl;
}
```

---

## 스레드 안전성

모든 public 메서드는 스레드 안전하며 여러 스레드에서 동시에 호출할 수 있습니다.

---

## 관련 문서

- [API Reference (English)](API_REFERENCE.md)
- [빠른 시작 가이드](getting_started/)
- [예제](EXAMPLES.md)
- [버전 호환성](advanced/VERSION_COMPATIBILITY.md)
