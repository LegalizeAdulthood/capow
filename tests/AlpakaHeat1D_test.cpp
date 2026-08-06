#include "AlpakaBackend.hpp"
#include "AlpakaHeat1D.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

void ExpectHeatGpuMatchesHost(int width, int steps, capow::AlpakaPlaneValue tolerance)
{
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.IsGpuAvailable())
    {
        GTEST_SKIP() << manager.GetAvailabilityMessage();
    }

    capow::Heat1DOptions options;
    options.width = width;
    options.steps = steps;
    options.dtOverDx2 = 0.1875F;
    options.heatIncrement = 0.375F;
    options.maxIntensity = 7.0F;
    options.maxVelocity = 5.0F;
    options.timeStep = 0.25F;

    std::vector<capow::AlpakaPlaneValue> initial;
    capow::Heat1DFields hostResult;
    capow::Heat1DFields gpuResult;
    capow::MakeHeat1DInitial(options, &initial);
    capow::RunHeat1DHost(options, initial, &hostResult);
    capow::RunHeat1DGpu(options, initial, &gpuResult);

    const capow::AlpakaPlaneValue intensityError =
        capow::MaxHeat1DDifference(hostResult.intensityField, gpuResult.intensityField);
    const capow::AlpakaPlaneValue velocityError =
        capow::MaxHeat1DDifference(hostResult.velocityField, gpuResult.velocityField);
    EXPECT_LE(intensityError, tolerance) << "max intensity error: " << intensityError;
    EXPECT_LE(velocityError, tolerance) << "max velocity error: " << velocityError;
}

} // namespace

TEST(alpakaHeat1D, oneStepMatchesHost)
{
    ExpectHeatGpuMatchesHost(19, 1, 1.0e-5F);
}

TEST(alpakaHeat1D, thousandStepsMatchHost)
{
    ExpectHeatGpuMatchesHost(257, 1000, 2.5e-5F);
}
