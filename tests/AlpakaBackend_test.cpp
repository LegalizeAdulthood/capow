#include "AlpakaBackend.hpp"
#include "resource.h"

#include <gtest/gtest.h>

namespace
{

struct WorldComboLiveRule
{
    int caType;
    capow::AlpakaLiveRuleKind kind;
    capow::AlpakaRule rule;
};

const WorldComboLiveRule kWorldComboLiveRules[] = {
    {CA_STANDARD, capow::ALPAKA_LIVE_RULE_DIGITAL_1D, capow::ALPAKA_RULE_CA_STANDARD},
    {CA_REVERSIBLE, capow::ALPAKA_LIVE_RULE_DIGITAL_1D, capow::ALPAKA_RULE_CA_REVERSIBLE},
    {CA_HEATWAVE, capow::ALPAKA_LIVE_RULE_HEAT_1D, capow::ALPAKA_RULE_CA_HEATWAVE},
    {CA_HEATWAVE2, capow::ALPAKA_LIVE_RULE_HEAT_1D, capow::ALPAKA_RULE_CA_HEATWAVE2},
    {ALT_CA_WAVE, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_CA_WAVE},
    {CA_WAVE2, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_CA_WAVE2},
    {CA_OSCILLATOR, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_CA_OSCILLATOR},
    {CA_DIVERSE_OSCILLATOR, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_CA_DIVERSE_OSCILLATOR},
    {ALT_CA_OSCILLATOR_WAVE, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_ALT_CA_OSCILLATOR_WAVE},
    {ALT_CA_DIVERSE_OSCILLATOR_WAVE, capow::ALPAKA_LIVE_RULE_WAVE_1D,
        capow::ALPAKA_RULE_ALT_CA_DIVERSE_OSCILLATOR_WAVE},
    {ALT_CA_ULAM_WAVE, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_CA_ULAM_WAVE},
    {CA_CUBIC_ULAM_WAVE, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_CA_CUBIC_ULAM_WAVE},
    {CA_AUTO_ULAM_WAVE, capow::ALPAKA_LIVE_RULE_WAVE_1D, capow::ALPAKA_RULE_CA_AUTO_ULAM_WAVE},
    {CA_WAVE_2D, capow::ALPAKA_LIVE_RULE_PLANE_2D, capow::ALPAKA_RULE_CA_WAVE_2D},
    {CA_HEAT_2D, capow::ALPAKA_LIVE_RULE_PLANE_2D, capow::ALPAKA_RULE_CA_HEAT_2D},
};

} // namespace

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

TEST(alpakaBackend, waveStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_WAVE));
}

TEST(alpakaBackend, wave2StartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_WAVE2));
}

TEST(alpakaBackend, oscillatorStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_OSCILLATOR));
}

TEST(alpakaBackend, diverseOscillatorStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_DIVERSE_OSCILLATOR));
}

TEST(alpakaBackend, oscillatorWaveStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_ALT_CA_OSCILLATOR_WAVE));
}

TEST(alpakaBackend, diverseOscillatorWaveStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_ALT_CA_DIVERSE_OSCILLATOR_WAVE));
}

TEST(alpakaBackend, ulamRulesStartEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_ULAM_WAVE));
    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_AUTO_ULAM_WAVE));
    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_CUBIC_ULAM_WAVE));
}

TEST(alpakaBackend, standardStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_STANDARD));
}

TEST(alpakaBackend, reversibleStartsEnabled)
{
    const capow::AlpakaManager manager;

    EXPECT_TRUE(manager.IsRuleEnabled(capow::ALPAKA_RULE_CA_REVERSIBLE));
}

TEST(alpakaBackend, gpuRunsRequireDeviceAndEnabledRule)
{
    capow::AlpakaManager manager;

    manager.SetRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D, false);
    EXPECT_FALSE(manager.CanRunGpu(capow::ALPAKA_RULE_CA_HEAT_2D));

    manager.SetRuleEnabled(capow::ALPAKA_RULE_CA_HEAT_2D, true);

    EXPECT_EQ(manager.IsGpuAvailable(), manager.CanRunGpu(capow::ALPAKA_RULE_CA_HEAT_2D));
}

TEST(alpakaBackend, worldComboBuiltinsMapToLiveGpuRules)
{
    for (const WorldComboLiveRule &expected : kWorldComboLiveRules)
    {
        capow::AlpakaLiveRule actual = {capow::ALPAKA_LIVE_RULE_NONE, capow::ALPAKA_RULE_COUNT};

        EXPECT_TRUE(capow::GetAlpakaLiveRuleForCaType(expected.caType, &actual)) << expected.caType;
        EXPECT_EQ(expected.kind, actual.kind) << expected.caType;
        EXPECT_EQ(expected.rule, actual.rule) << expected.caType;
    }
}

TEST(alpakaBackend, worldComboBuiltinsStartEnabled)
{
    const capow::AlpakaManager manager;

    for (const WorldComboLiveRule &entry : kWorldComboLiveRules)
    {
        EXPECT_TRUE(manager.IsRuleEnabled(entry.rule)) << capow::AlpakaRuleName(entry.rule);
    }
}

TEST(alpakaBackend, worldComboUserRuleIsCpuOnly)
{
    capow::AlpakaLiveRule liveRule = {capow::ALPAKA_LIVE_RULE_NONE, capow::ALPAKA_RULE_COUNT};

    EXPECT_FALSE(capow::GetAlpakaLiveRuleForCaType(CA_USER, &liveRule));
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
    EXPECT_STREQ("CA_WAVE", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_WAVE));
    EXPECT_STREQ("CA_WAVE2", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_WAVE2));
    EXPECT_STREQ("CA_OSCILLATOR", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_OSCILLATOR));
    EXPECT_STREQ("CA_DIVERSE_OSCILLATOR", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_DIVERSE_OSCILLATOR));
    EXPECT_STREQ("ALT_CA_OSCILLATOR_WAVE", capow::AlpakaRuleName(capow::ALPAKA_RULE_ALT_CA_OSCILLATOR_WAVE));
    EXPECT_STREQ(
        "ALT_CA_DIVERSE_OSCILLATOR_WAVE", capow::AlpakaRuleName(capow::ALPAKA_RULE_ALT_CA_DIVERSE_OSCILLATOR_WAVE));
    EXPECT_STREQ("CA_ULAM_WAVE", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_ULAM_WAVE));
    EXPECT_STREQ("CA_AUTO_ULAM_WAVE", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_AUTO_ULAM_WAVE));
    EXPECT_STREQ("CA_CUBIC_ULAM_WAVE", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_CUBIC_ULAM_WAVE));
    EXPECT_STREQ("CA_STANDARD", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_STANDARD));
    EXPECT_STREQ("CA_REVERSIBLE", capow::AlpakaRuleName(capow::ALPAKA_RULE_CA_REVERSIBLE));
}
