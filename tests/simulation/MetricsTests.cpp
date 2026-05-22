#include "simulation/core/Simulation.hpp"
#include "simulation/metrics/CrossDomainInteraction.hpp"

#include <gtest/gtest.h>

#include <algorithm>

TEST(MetricsAggregatorTests, ComputesGlobalMetricsFromFrontendSignals)
{
    MetricsSnapshot snapshot;
    snapshot.averageLatencySeconds = 1.5;
    snapshot.inputRatePerSecond = 24.0;
    snapshot.processedPerSecond = 12.0;
    snapshot.retryRatePerSecond = 2.0;
    snapshot.timeoutRatePerSecond = 1.0;
    snapshot.apiQueueDepth = 4;
    snapshot.databaseQueueDepth = 6;
    snapshot.apiUtilization = 0.8;
    snapshot.databaseUtilization = 0.6;
    snapshot.cacheHitRate = 0.25;
    snapshot.totalCacheLookups = 10;
    snapshot.observability.enabledSystemCount = 7;
    snapshot.complexity.current = 2.0;
    snapshot.complexity.recommendedThreshold = 10.0;

    MetricsAggregator::update(snapshot);

    EXPECT_GT(snapshot.frontend.perceivedLatency, snapshot.averageLatencySeconds);
    EXPECT_GT(snapshot.frontend.framePressure, 0.0);
    EXPECT_LT(snapshot.global.userExperience, 100.0);
    EXPECT_GT(snapshot.global.infrastructurePressure, 0.0);
    EXPECT_LT(snapshot.global.reliability, 100.0);
    EXPECT_GT(snapshot.global.complexity, 20.0);
    EXPECT_FALSE(snapshot.contributions.empty());
}

TEST(MetricsAggregatorTests, HiddenPressureStateDerivesSpecializedMetrics)
{
    MetricsSnapshot snapshot;
    snapshot.averageLatencySeconds = 0.2;
    snapshot.inputRatePerSecond = 8.0;
    snapshot.processedPerSecond = 8.0;
    snapshot.pressureState.frontend.assetWeight = 0.9;
    snapshot.pressureState.frontend.renderComplexity = 0.8;
    snapshot.pressureState.frontend.realtimeIntensity = 0.7;
    snapshot.pressureState.frontend.sessionPersistence = 0.2;
    snapshot.pressureState.frontend.mobileCompatibility = 0.3;
    snapshot.pressureState.backend.serviceFragmentation = 0.6;
    snapshot.pressureState.network.bandwidthPressure = 0.7;
    snapshot.pressureState.network.trafficBurstiness = 0.8;
    snapshot.pressureState.database.readPressure = 0.7;
    snapshot.pressureState.database.contention = 0.8;
    snapshot.pressureState.database.replicationLag = 0.5;
    snapshot.pressureState.runtime.cpuPressure = 0.7;
    snapshot.pressureState.runtime.memoryPressure = 0.6;
    snapshot.pressureState.runtime.schedulingPressure = 0.5;

    MetricsAggregator::update(snapshot);

    EXPECT_GT(snapshot.frontend.assetBandwidth, 40.0);
    EXPECT_GT(snapshot.frontend.websocketPressure, 40.0);
    EXPECT_GT(snapshot.frontend.sessionStalenessRisk, 20.0);
    EXPECT_GT(snapshot.backend.serviceFragmentation, 50.0);
    EXPECT_GT(snapshot.network.deliveryPressure, 40.0);
    EXPECT_GT(snapshot.database.persistenceRisk, 40.0);
    EXPECT_GT(snapshot.runtime.executionRisk, 40.0);
    EXPECT_GT(snapshot.global.infrastructurePressure, 20.0);
    EXPECT_TRUE(std::any_of(snapshot.contributions.begin(), snapshot.contributions.end(), [](const MetricContribution& contribution) {
        return contribution.domain == MetricContributionDomain::Database;
    }));
    EXPECT_TRUE(std::any_of(snapshot.contributions.begin(), snapshot.contributions.end(), [](const MetricContribution& contribution) {
        return contribution.domain == MetricContributionDomain::Runtime;
    }));
}

TEST(CrossDomainInteractionTests, FrontendRealtimeCreatesBackendNetworkAndRuntimePressure)
{
    PressureState state;
    state.frontend.realtimeIntensity = 0.8;

    const PressureState propagated = CrossDomainInteraction::apply(state);

    EXPECT_GT(propagated.backend.requestLoad, state.backend.requestLoad);
    EXPECT_GT(propagated.network.bandwidthPressure, state.network.bandwidthPressure);
    EXPECT_GT(propagated.runtime.cpuPressure, state.runtime.cpuPressure);
}

TEST(CrossDomainInteractionTests, PropagationClampsWithoutRunawayValues)
{
    PressureState state;
    state.frontend.realtimeIntensity = 1.0;
    state.frontend.assetWeight = 1.0;
    state.backend.queuePressure = 1.0;
    state.backend.serviceFragmentation = 1.0;
    state.network.trafficBurstiness = 1.0;
    state.database.contention = 1.0;
    state.runtime.memoryPressure = 1.0;

    for (int i = 0; i < 100; ++i) {
        state = CrossDomainInteraction::apply(state);
    }

    EXPECT_LE(state.backend.requestLoad, 1.0);
    EXPECT_LE(state.backend.queuePressure, 1.0);
    EXPECT_LE(state.network.bandwidthPressure, 1.0);
    EXPECT_LE(state.runtime.schedulingPressure, 1.0);
    EXPECT_LE(state.database.replicationLag, 1.0);
}

TEST(CrossDomainInteractionTests, DatabaseContentionFlowsIntoFrontendLatencyThroughBackendQueues)
{
    PressureState state;
    state.database.contention = 0.8;

    MetricsSnapshot baseline;
    baseline.pressureState = state;
    MetricsAggregator::update(baseline);

    MetricsSnapshot propagated;
    propagated.pressureState = CrossDomainInteraction::apply(state);
    MetricsAggregator::update(propagated);

    EXPECT_GT(propagated.pressureState.backend.queuePressure, baseline.pressureState.backend.queuePressure);
    EXPECT_GT(propagated.frontend.perceivedLatency, baseline.frontend.perceivedLatency);
    EXPECT_LT(propagated.global.userExperience, baseline.global.userExperience);
}
