#include "AlpakaBackend.hpp"
#include "AlpakaDigital.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

void ExpectStandardGpuMatchesHost(int width, int steps, int stateCount, int radius)
{
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.IsGpuAvailable())
    {
        GTEST_SKIP() << manager.GetAvailabilityMessage();
    }

    capow::StandardDigitalOptions options;
    options.width = width;
    options.steps = steps;
    options.stateCount = stateCount;
    options.radius = radius;
    options.stateBits = stateCount == 16 ? 4 : stateCount == 4 ? 2 : 1;
    options.lookupCount = 1;
    for (int i = 0; i < 1 + 2 * radius; ++i)
    {
        options.lookupCount *= stateCount;
    }

    std::vector<capow::AlpakaDigitalValue> source;
    std::vector<capow::AlpakaDigitalValue> lookup;
    std::vector<capow::AlpakaDigitalValue> hostResult;
    std::vector<capow::AlpakaDigitalValue> gpuResult;
    capow::MakeStandardDigitalInitial(options, &source);
    capow::MakeStandardDigitalLookup(options, &lookup);
    capow::RunStandardDigitalHost(options, source, lookup, &hostResult);
    capow::RunStandardDigitalGpu(options, source, lookup, &gpuResult);

    EXPECT_EQ(0, capow::CountStandardDigitalDifferences(hostResult, gpuResult));
}

} // namespace

TEST(alpakaDigital, oneStepMatchesHost)
{
    ExpectStandardGpuMatchesHost(31, 1, 16, 1);
}

TEST(alpakaDigital, fiftyStepsMatchHost)
{
    ExpectStandardGpuMatchesHost(257, 50, 16, 1);
}

TEST(alpakaDigital, widerRadiusMatchesHost)
{
    ExpectStandardGpuMatchesHost(263, 50, 4, 2);
}
