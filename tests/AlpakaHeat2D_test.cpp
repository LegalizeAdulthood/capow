#include "AlpakaBackend.hpp"
#include "AlpakaHeat2D.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

void ExpectHeatGpuMatchesHost(int width, int height, int steps)
{
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.IsGpuAvailable())
    {
        GTEST_SKIP() << manager.GetAvailabilityMessage();
    }

    capow::Heat2DOptions options;
    options.width = width;
    options.height = height;
    options.steps = steps;
    options.heatIncrement = 0.375F;
    options.maxIntensity = 7.0F;
    options.timeStep = 0.25F;

    std::vector<capow::AlpakaPlaneValue> initial;
    capow::Heat2DFields hostResult;
    capow::Heat2DFields gpuResult;
    capow::MakeHeat2DInitial(options, &initial);
    capow::RunHeat2DHost(options, initial, &hostResult);
    capow::RunHeat2DGpu(options, initial, &gpuResult);

    const capow::AlpakaPlaneValue intensityError =
        capow::MaxHeat2DDifference(hostResult.intensity, gpuResult.intensity);
    const capow::AlpakaPlaneValue velocityError = capow::MaxHeat2DDifference(hostResult.velocity, gpuResult.velocity);
    EXPECT_LE(intensityError, 1.0e-5F) << "max intensity error: " << intensityError;
    EXPECT_LE(velocityError, 1.0e-5F) << "max velocity error: " << velocityError;
}

} // namespace

TEST(alpakaHeat2D, oneStepMatchesHost)
{
    ExpectHeatGpuMatchesHost(19, 11, 1);
}

TEST(alpakaHeat2D, thousandStepsMatchHost)
{
    ExpectHeatGpuMatchesHost(37, 23, 1000);
}
