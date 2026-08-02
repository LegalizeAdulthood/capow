#include "AlpakaBackend.hpp"
#include "AlpakaSyntheticHeat.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

void ExpectGpuMatchesHost(int width, int height, int steps)
{
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.CanRunGpu(capow::ALPAKA_RULE_SYNTHETIC_HEAT_2D))
    {
        GTEST_SKIP() << manager.GetAvailabilityMessage();
    }

    capow::SyntheticHeat2DOptions options;
    options.width = width;
    options.height = height;
    options.steps = steps;

    std::vector<capow::AlpakaPlaneValue> initial;
    std::vector<capow::AlpakaPlaneValue> hostResult;
    std::vector<capow::AlpakaPlaneValue> gpuResult;
    capow::MakeSyntheticHeat2DInitial(options, &initial);
    capow::RunSyntheticHeat2DHost(options, initial, &hostResult);
    capow::RunSyntheticHeat2DGpu(options, initial, &gpuResult);

    EXPECT_EQ(hostResult.size(), gpuResult.size());
    EXPECT_LE(capow::MaxAbsDifference(hostResult, gpuResult), 1.0e-5F);
}

} // namespace

TEST(alpakaSyntheticHeat, oneStepMatchesHost)
{
    ExpectGpuMatchesHost(17, 9, 1);
}

TEST(alpakaSyntheticHeat, thousandStepsMatchHost)
{
    ExpectGpuMatchesHost(31, 13, 1000);
}
