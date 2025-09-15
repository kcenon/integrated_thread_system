/**
 * @file bench_throughput.cpp
 * @brief Throughput benchmarks for different system configurations
 */

#include <benchmark/benchmark.h>
#include "unified_thread_system.h"
#include <vector>

using namespace integrated_thread_system;

// Benchmark thread-only configuration
static void BM_ThreadOnly(benchmark::State& state) {
    config cfg;
    cfg.enable_thread_system(true)
       .enable_logger_system(false)
       .enable_monitoring_system(false)
       .set_worker_count(state.range(0));

    unified_thread_system system(cfg);

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 100; ++i) {
            futures.push_back(system.submit([i]() {
                return i * 2;
            }));
        }

        for (auto& f : futures) {
            benchmark::DoNotOptimize(f.get());
        }
    }

    state.SetItemsProcessed(state.iterations() * 100);
}

// Benchmark thread + logger configuration
static void BM_ThreadLogger(benchmark::State& state) {
    config cfg;
    cfg.enable_thread_system(true)
       .enable_logger_system(true)
       .enable_monitoring_system(false)
       .set_worker_count(state.range(0))
       .set_log_file("/dev/null");  // Discard logs

    unified_thread_system system(cfg);

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 100; ++i) {
            futures.push_back(system.submit([&system, i]() {
                system.log_debug("Task " + std::to_string(i));
                return i * 2;
            }));
        }

        for (auto& f : futures) {
            benchmark::DoNotOptimize(f.get());
        }
    }

    state.SetItemsProcessed(state.iterations() * 100);
}

// Benchmark all systems configuration
static void BM_AllSystems(benchmark::State& state) {
    config cfg;
    cfg.enable_all_systems()
       .set_worker_count(state.range(0))
       .set_log_file("/dev/null");

    unified_thread_system system(cfg);
    system.register_metric("tasks", metric_type::counter);

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 100; ++i) {
            futures.push_back(system.submit([&system, i]() {
                system.log_debug("Task " + std::to_string(i));
                system.increment_counter("tasks");
                return i * 2;
            }));
        }

        for (auto& f : futures) {
            benchmark::DoNotOptimize(f.get());
        }
    }

    state.SetItemsProcessed(state.iterations() * 100);
}

// Benchmark priority scheduling
static void BM_PriorityScheduling(benchmark::State& state) {
    config cfg;
    cfg.enable_thread_system(true)
       .enable_logger_system(false)
       .enable_monitoring_system(false)
       .set_worker_count(state.range(0));

    unified_thread_system system(cfg);

    for (auto _ : state) {
        std::vector<std::future<int>> futures;

        // Mix of priorities
        for (int i = 0; i < 30; ++i) {
            futures.push_back(system.submit_critical([i]() { return i * 3; }));
        }
        for (int i = 0; i < 40; ++i) {
            futures.push_back(system.submit([i]() { return i * 2; }));
        }
        for (int i = 0; i < 30; ++i) {
            futures.push_back(system.submit_background([i]() { return i; }));
        }

        for (auto& f : futures) {
            benchmark::DoNotOptimize(f.get());
        }
    }

    state.SetItemsProcessed(state.iterations() * 100);
}

// Benchmark with different worker counts
BENCHMARK(BM_ThreadOnly)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
BENCHMARK(BM_ThreadLogger)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
BENCHMARK(BM_AllSystems)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);
BENCHMARK(BM_PriorityScheduling)->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16);

// Benchmark single task submission latency
static void BM_SubmissionLatency(benchmark::State& state) {
    config cfg;
    cfg.enable_thread_system(true)
       .enable_logger_system(false)
       .enable_monitoring_system(false)
       .set_worker_count(4);

    unified_thread_system system(cfg);

    for (auto _ : state) {
        auto future = system.submit([]() { return 42; });
        benchmark::DoNotOptimize(future.get());
    }
}

BENCHMARK(BM_SubmissionLatency);

// Benchmark monitoring overhead
static void BM_MonitoringOverhead(benchmark::State& state) {
    config cfg;
    cfg.enable_thread_system(false)
       .enable_logger_system(false)
       .enable_monitoring_system(true);

    unified_thread_system system(cfg);
    system.register_metric("test_counter", metric_type::counter);
    system.register_metric("test_gauge", metric_type::gauge);

    for (auto _ : state) {
        system.increment_counter("test_counter");
        system.set_gauge("test_gauge", 42.0);
        benchmark::DoNotOptimize(system.get_counter("test_counter"));
    }

    state.SetItemsProcessed(state.iterations() * 3);  // 3 operations per iteration
}

BENCHMARK(BM_MonitoringOverhead);

// Benchmark logging overhead
static void BM_LoggingOverhead(benchmark::State& state) {
    config cfg;
    cfg.enable_thread_system(false)
       .enable_logger_system(true)
       .enable_monitoring_system(false)
       .set_log_file("/dev/null");

    unified_thread_system system(cfg);

    for (auto _ : state) {
        system.log_info("Benchmark log message");
    }

    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_LoggingOverhead);

// Benchmark batch submission
static void BM_BatchSubmission(benchmark::State& state) {
    const int batch_size = state.range(0);

    config cfg;
    cfg.enable_thread_system(true)
       .enable_logger_system(false)
       .enable_monitoring_system(false)
       .set_worker_count(4);

    unified_thread_system system(cfg);

    for (auto _ : state) {
        std::vector<std::future<int>> futures;
        futures.reserve(batch_size);

        // Submit batch
        for (int i = 0; i < batch_size; ++i) {
            futures.push_back(system.submit([i]() { return i; }));
        }

        // Wait for all
        for (auto& f : futures) {
            benchmark::DoNotOptimize(f.get());
        }
    }

    state.SetItemsProcessed(state.iterations() * batch_size);
}

BENCHMARK(BM_BatchSubmission)->Arg(10)->Arg(100)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();