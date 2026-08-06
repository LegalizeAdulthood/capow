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

TEST(alpakaBackend, core2DRulesStartEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D));
    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_WAVE_2D));
}

TEST(alpakaBackend, syntheticHeatStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_SYNTHETIC_HEAT_2D));
}

TEST(alpakaBackend, heat2DStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D));
}

TEST(alpakaBackend, heatwaveStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_HEATWAVE));
}

TEST(alpakaBackend, heatwave2StartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_HEATWAVE2));
}

TEST(alpakaBackend, gpuRunsRequireDeviceAndEnabledRule)
{
    capow::AlpakaManager manager;

    manager.SetRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D, false);
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
    EXPECT_STREQ("CA_HEATWAVE", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_HEATWAVE));
    EXPECT_STREQ("CA_HEATWAVE2", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_HEATWAVE2));
}
