# Performance Baseline

**Version**: 1.0
**Last Updated**: 2025-11-30

---

## Overview

This document establishes performance baselines for integrated_thread_system to enable regression detection and optimization tracking.

---

## Baseline Metrics

### Thread Pool Performance

| Metric | Baseline | Acceptable Range |
|--------|----------|------------------|
| Throughput (8 workers) | 2.1M jobs/s | >1.8M |
| Latency p50 | 0.4 ms | <0.6 ms |
| Latency p99 | 1.2 ms | <2.0 ms |
| Scaling efficiency (8 cores) | 96% | >90% |

### Logging Performance

| Metric | Baseline | Acceptable Range |
|--------|----------|------------------|
| Throughput (async) | 4.34M logs/s | >3.5M |
| Latency (avg) | 250 ns | <400 ns |
| Memory per 1M logs | 25 MB | <35 MB |

### Monitoring Performance

| Metric | Baseline | Acceptable Range |
|--------|----------|------------------|
| CPU overhead | <1% | <2% |
| Collection latency | 100 ns | <200 ns |
| Memory usage | 1.5 MB | <3 MB |

---

## Test Environment

### Reference Platform

- **CPU**: Apple M1 (8-core) @ 3.2GHz
- **RAM**: 16GB
- **OS**: macOS Sonoma
- **Compiler**: Apple Clang 15, -O3

### CI Platform

- **CPU**: GitHub Actions runner (2-core)
- **RAM**: 7GB
- **OS**: Ubuntu 22.04
- **Compiler**: GCC 11, -O2

---

## Benchmark Commands

```bash
# Run all benchmarks
./build/benchmarks/performance_benchmark

# Thread pool only
./build/benchmarks/performance_benchmark --benchmark_filter="ThreadPool*"

# With JSON output
./build/benchmarks/performance_benchmark --benchmark_format=json > results.json
```

---

## Regression Detection

### Automatic Alerts

CI will fail if any metric degrades beyond acceptable range.

### Manual Verification

```bash
# Compare with baseline
python scripts/compare_benchmarks.py baseline.json current.json
```

---

## Historical Data

Performance history is tracked in CI artifacts and can be viewed in the GitHub Actions dashboard.
