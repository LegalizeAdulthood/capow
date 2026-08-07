#include "AlpakaBackend.hpp"
#include "AlpakaWave1D.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

void ExpectWaveGpuMatchesHost(int width, int steps, capow::Wave1DRule rule, capow::AlpakaPlaneValue tolerance)
{
    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.IsGpuAvailable())
    {
        GTEST_SKIP() << manager.GetAvailabilityMessage();
    }

    capow::Wave1DOptions options;
    options.width = width;
    options.steps = steps;
    options.rule = rule;
    options.waveSpeed2TimeStep2OverDx2 = 0.375F;
    options.dtOver12Dx2 = 0.03125F;
    options.maxIntensity = 7.0F;
    options.maxVelocity = 5.0F;
    options.timeStep = 0.25F;
    options.dtOverMass = 0.125F;
    options.frictionMultiplier = 0.25F;
    options.springMultiplier = 0.75F;
    options.driverValue = 0.5F;

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
    ExpectWaveGpuMatchesHost(19, 1, capow::WAVE_1D_RULE_THREE_NEIGHBOR, 1.0e-5F);
}

TEST(alpakaWave1D, fiftyStepsMatchHost)
{
    ExpectWaveGpuMatchesHost(257, 50, capow::WAVE_1D_RULE_THREE_NEIGHBOR, 2.5e-5F);
}

TEST(alpakaWave1D, fiveNeighborOneStepMatchesHost)
{
    ExpectWaveGpuMatchesHost(23, 1, capow::WAVE_1D_RULE_FIVE_NEIGHBOR, 1.0e-5F);
}

TEST(alpakaWave1D, fiveNeighborFiftyStepsMatchHost)
{
    ExpectWaveGpuMatchesHost(263, 50, capow::WAVE_1D_RULE_FIVE_NEIGHBOR, 2.5e-5F);
}

TEST(alpakaWave1D, oscillatorFiftyStepsMatchHost)
{
    ExpectWaveGpuMatchesHost(257, 50, capow::WAVE_1D_RULE_OSCILLATOR, 2.5e-5F);
}

TEST(alpakaWave1D, diverseOscillatorFiftyStepsMatchHost)
{
    ExpectWaveGpuMatchesHost(257, 50, capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR, 2.5e-5F);
}
