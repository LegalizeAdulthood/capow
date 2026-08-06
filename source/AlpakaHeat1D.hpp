#ifndef ALPAKAHEAT1D_HPP
#define ALPAKAHEAT1D_HPP

#include "AlpakaBuffers.hpp"
#include "AlpakaTiming.hpp"
#include "CapowRules.hpp"

#include <vector>

namespace capow
{

struct Heat1DOptions
{
    int width;
    int steps;
    Heat1DRule rule;
    AlpakaPlaneValue dtOverDx2;
    AlpakaPlaneValue heatIncrement;
    AlpakaPlaneValue maxIntensity;
    AlpakaPlaneValue maxVelocity;
    AlpakaPlaneValue timeStep;

    Heat1DOptions();
};

struct Heat1DFields
{
    std::vector<AlpakaPlaneValue> intensityField;
    std::vector<AlpakaPlaneValue> velocityField;
};

void MakeHeat1DInitial(const Heat1DOptions &options, std::vector<AlpakaPlaneValue> *field);
void RunHeat1DHost(const Heat1DOptions &options, const std::vector<AlpakaPlaneValue> &initial, Heat1DFields *result);
void RunHeat1DGpu(const Heat1DOptions &options, const std::vector<AlpakaPlaneValue> &initial, Heat1DFields *result);
void RunHeat1DGpuTimed(const Heat1DOptions &options, const std::vector<AlpakaPlaneValue> &initial, Heat1DFields *result,
    AlpakaTimingMeasurements *timing);
AlpakaPlaneValue MaxHeat1DDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual);

} // namespace capow

#endif
