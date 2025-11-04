// BSD 3-Clause License
// Copyright (c) 2025, kcenon

#include <gtest/gtest.h>
#include <kcenon/integrated/extensions/metrics_aggregator.h>
#include <kcenon/integrated/adapters/thread_adapter.h>

using namespace kcenon::integrated;

class MetricsAggregatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        aggregator = std::make_unique<extensions::metrics_aggregator>();
    }

    std::unique_ptr<extensions::metrics_aggregator> aggregator;
};

TEST_F(MetricsAggregatorTest, InitializeAndShutdown) {
    EXPECT_TRUE(aggregator->initialize().is_ok());
    EXPECT_TRUE(aggregator->shutdown().is_ok());
}

TEST_F(MetricsAggregatorTest, CollectMetricsWithoutAdapters) {
    ASSERT_TRUE(aggregator->initialize().is_ok());

    auto result = aggregator->collect_metrics();
    ASSERT_TRUE(result.is_ok());

    const auto& metrics = result.value();
    EXPECT_EQ(metrics.thread_pool_workers, 0);
    EXPECT_EQ(metrics.thread_pool_queue_size, 0);
}

TEST_F(MetricsAggregatorTest, PrometheusFormatExport) {
    ASSERT_TRUE(aggregator->initialize().is_ok());
    aggregator->collect_metrics();

    std::string prometheus = aggregator->export_prometheus_format();

    // Check for essential Prometheus format elements
    EXPECT_NE(prometheus.find("# HELP"), std::string::npos);
    EXPECT_NE(prometheus.find("# TYPE"), std::string::npos);
    EXPECT_NE(prometheus.find("thread_pool_workers"), std::string::npos);
    EXPECT_NE(prometheus.find("system_cpu_usage_percent"), std::string::npos);
}

TEST_F(MetricsAggregatorTest, JsonFormatExport) {
    ASSERT_TRUE(aggregator->initialize().is_ok());
    aggregator->collect_metrics();

    std::string json = aggregator->export_json_format();

    // Check for essential JSON elements
    EXPECT_NE(json.find("\"timestamp\""), std::string::npos);
    EXPECT_NE(json.find("\"thread_pool\""), std::string::npos);
    EXPECT_NE(json.find("\"system\""), std::string::npos);
    EXPECT_NE(json.find("\"logger\""), std::string::npos);
}

TEST_F(MetricsAggregatorTest, CollectMetricsBeforeInitialize) {
    auto result = aggregator->collect_metrics();
    EXPECT_TRUE(result.has_error());
}
