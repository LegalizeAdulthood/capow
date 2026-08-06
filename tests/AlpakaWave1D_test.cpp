#include "AlpakaBackend.hpp"
#include "AlpakaWave1D.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

void ExpectWaveGpuMatchesHost(int width, int steps, capow::AlpakaPlaneValue tolerance)
{
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.IsGpuAvailable())
    {
        GTEST_SKIP() << manager.GetAvailabilityMessage();
    }

    capow::Wave1DOptions options;
    options.width = width;
    options.steps = steps;
    options.waveSpeed2TimeStep2OverDx2 = 0.375F;
    options.maxIntensity = 7.0F;
    options.timeStep = 0.25F;

    std::vector<capow::AlpakaPlaneValue> source;
    std::vector<capow::AlpakaPlaneValue> past;
    capow::Wave1DFields hostResult;
    capow::Wave1DFields gpuResult;
    capow::MakeWave1DInitial(options, &source, &past);
    capow::RunWave1DHost(options, source, past, &hostResult);
    capow::RunWave1DGpu(options, source, past, &gpuResult);

    const capow::AlpakaPlaneValue intensityError =
        capow::MaxWave1DDifference(hostResult.intensityField, gpuResult.intensityField);
    const capow::AlpakaPlaneValue velocityError =
        capow::MaxWave1DDifference(hostResult.velocityField, gpuResult.velocityField);
    EXPECT_LE(intensityError, tolerance) << "max intensity error: " << intensityError;
    EXPECT_LE(velocityError, tolerance) << "max velocity error: " << velocityError;
}

} // namespace

TEST(alpakaWave1D, oneStepMatchesHost)
{
    ExpectWaveGpuMatchesHost(19, 1, 1.0e-5F);
}

TEST(alpakaWave1D, fiftyStepsMatchHost)
{
    ExpectWaveGpuMatchesHost(257, 50, 2.5e-5F);
}
