#ifndef ALPAKAWAVE1D_HPP
#define ALPAKAWAVE1D_HPP

#include "AlpakaBuffers.hpp"
#include "AlpakaTiming.hpp"
#include "CapowRules.hpp"

#include <vector>

namespace capow
{

struct Wave1DOptions
{
    int width;
    int steps;
    Wave1DRule rule;
    AlpakaPlaneValue waveSpeed2TimeStep2OverDx2;
    AlpakaPlaneValue dtOver12Dx2;
    AlpakaPlaneValue dtOverDx2;
    AlpakaPlaneValue maxIntensity;
    AlpakaPlaneValue maxVelocity;
    AlpakaPlaneValue timeStep;
    AlpakaPlaneValue dtOverMass;
    AlpakaPlaneValue frictionMultiplier;
    AlpakaPlaneValue springMultiplier;
    AlpakaPlaneValue driverValue;
    AlpakaPlaneValue nonlinearity1;
    AlpakaPlaneValue nonlinearity2;

    Wave1DOptions();
};

struct Wave1DFields
{
    std::vector<AlpakaPlaneValue> intensityField;
    std::vector<AlpakaPlaneValue> velocityField;
};

void MakeWave1DInitial(
    const Wave1DOptions &options, std::vector<AlpakaPlaneValue> *source, std::vector<AlpakaPlaneValue> *past);
void RunWave1DHost(const Wave1DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave1DFields *result);
void RunWave1DGpu(const Wave1DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave1DFields *result);
void RunWave1DGpuTimed(const Wave1DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave1DFields *result, AlpakaTimingMeasurements *timing);
AlpakaPlaneValue MaxWave1DDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual);

} // namespace capow

#endif
