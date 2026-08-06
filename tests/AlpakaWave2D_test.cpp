#include "AlpakaBackend.hpp"
#include "AlpakaWave2D.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

void ExpectWaveGpuMatchesHost(int width, int height, int steps, capow::AlpakaPlaneValue tolerance)
{
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.IsGpuAvailable())
    {
        GTEST_SKIP() << manager.GetAvailabilityMessage();
    }

    capow::Wave2DOptions options;
    options.width = width;
    options.height = height;
    options.steps = steps;
    options.waveSpeed2TimeStep2OverDx2 = 0.375F;
    options.maxIntensity = 7.0F;
    options.timeStep = 0.25F;

    std::vector<capow::AlpakaPlaneValue> source;
    std::vector<capow::AlpakaPlaneValue> past;
    capow::Wave2DFields hostResult;
    capow::Wave2DFields gpuResult;
    capow::MakeWave2DInitial(options, &source, &past);
    capow::RunWave2DHost(options, source, past, &hostResult);
    capow::RunWave2DGpu(options, source, past, &gpuResult);

    const capow::AlpakaPlaneValue intensityError =
        capow::MaxWave2DDifference(hostResult.intensityField, gpuResult.intensityField);
    const capow::AlpakaPlaneValue velocityError =
        capow::MaxWave2DDifference(hostResult.velocityField, gpuResult.velocityField);
    EXPECT_LE(intensityError, tolerance) << "max intensity error: " << intensityError;
    EXPECT_LE(velocityError, tolerance) << "max velocity error: " << velocityError;
}

} // namespace

TEST(alpakaWave2D, oneStepMatchesHost)
{
    ExpectWaveGpuMatchesHost(19, 11, 1, 1.0e-5F);
}

TEST(alpakaWave2D, fiftyStepsMatchHost)
{
    ExpectWaveGpuMatchesHost(37, 23, 50, 2.5e-5F);
}
