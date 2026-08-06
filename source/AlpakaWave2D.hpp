#ifndef ALPAKAWAVE2D_HPP
#define ALPAKAWAVE2D_HPP

#include "AlpakaBuffers.hpp"

#include <vector>

namespace capow
{

struct Wave2DOptions
{
    int width;
    int height;
    int steps;
    AlpakaPlaneValue waveSpeed2TimeStep2OverDx2;
    AlpakaPlaneValue maxIntensity;
    AlpakaPlaneValue timeStep;

    Wave2DOptions();
};

struct Wave2DFields
{
    std::vector<AlpakaPlaneValue> intensityField;
    std::vector<AlpakaPlaneValue> velocityField;
};

void MakeWave2DInitial(
    const Wave2DOptions &options, std::vector<AlpakaPlaneValue> *source, std::vector<AlpakaPlaneValue> *past);
void RunWave2DHost(const Wave2DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave2DFields *result);
void RunWave2DGpu(const Wave2DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave2DFields *result);
AlpakaPlaneValue MaxWave2DDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual);

} // namespace capow

#endif
