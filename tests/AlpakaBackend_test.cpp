#include "AlpakaBackend.hpp"

#include <gtest/gtest.h>

TEST(alpakaBackend, defaultsToCpuBackend)
{
    const capow::AlpakaManager manager;

    EXPECT_EQ(capow::ALPAKA_BACKEND_CPU, manager.GetBackend());
}

TEST(alpakaBackend, canSelectCpuAndGpu)
{
    capow::AlpakaManager manager;

    manager.SetBackend(capow::ALPAKA_BACKEND_GPU);
    EXPECT_EQ(capow::ALPAKA_BACKEND_GPU, manager.GetBackend());

    manager.SetBackend(capow::ALPAKA_BACKEND_CPU);
    EXPECT_EQ(capow::ALPAKA_BACKEND_CPU, manager.GetBackend());
}

TEST(alpakaBackend, caRulesStartDisabledAndCanBeEnabled)
{
    capow::AlpakaManager manager;

    EXPECT_FALSE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D));
    EXPECT_FALSE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_WAVE_2D));

    manager.SetRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D, true);

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D));
    EXPECT_FALSE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_WAVE_2D));
}

TEST(alpakaBackend, syntheticHeatStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_SYNTHETIC_HEAT_2D));
}

TEST(alpakaBackend, gpuRunsRequireDeviceAndEnabledRule)
{
    capow::AlpakaManager manager;

    EXPECT_FALSE(manager.CanRunGpu(capow::ALPAKA_RULE_CA_HEAT_2D));

    manager.SetRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D, true);

    EXPECT_EQ(manager.IsGpuAvailable(), manager.CanRunGpu(capow::ALPAKA_RULE_CA_HEAT_2D));
}

TEST(alpakaBackend, namesMatchMenuAndRuleLabels)
{
    EXPECT_STREQ("CPU", capow::AlpakaBackendName(capow::ALPAKA_BACKEND_CPU));
    EXPECT_STREQ("GPU", capow::AlpakaBackendName(capow::ALPAKA_BACKEND_GPU));
    EXPECT_STREQ("SYNTHETIC_HEAT_2D", capow::AlpakaRuleName(capow::ALPAKA_RULE_SYNTHETIC_HEAT_2D));
    EXPECT_STREQ("CA_HEAT_2D", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_HEAT_2D));
    EXPECT_STREQ("CA_WAVE_2D", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_WAVE_2D));
}
