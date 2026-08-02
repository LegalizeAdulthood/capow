#ifndef ALPAKAHEAT2D_HPP
#define ALPAKAHEAT2D_HPP

#include "AlpakaBuffers.hpp"

#include <vector>

namespace capow
{

struct Heat2DOptions
{
    int width;
    int height;
    int steps;
    AlpakaPlaneValue heatIncrement;
    AlpakaPlaneValue maxIntensity;
    AlpakaPlaneValue timeStep;

    Heat2DOptions();
};

struct Heat2DFields
{
    std::vector<AlpakaPlaneValue> intensity;
    std::vector<AlpakaPlaneValue> velocity;
};

void MakeHeat2DInitial(const Heat2DOptions &options, std::vector<AlpakaPlaneValue> *field);
void RunHeat2DHost(const Heat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial, Heat2DFields *result);
void RunHeat2DGpu(const Heat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial, Heat2DFields *result);
AlpakaPlaneValue MaxHeat2DDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual);

} // namespace capow

#endif
