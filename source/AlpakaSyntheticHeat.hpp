#ifndef ALPAKASYNTHETICHEAT_HPP
#define ALPAKASYNTHETICHEAT_HPP

#include "AlpakaBuffers.hpp"

#include <vector>

namespace capow
{

struct SyntheticHeat2DOptions
{
    int width;
    int height;
    int steps;
    AlpakaPlaneValue diffusion;

    SyntheticHeat2DOptions();
};

void MakeSyntheticHeat2DInitial(const SyntheticHeat2DOptions &options, std::vector<AlpakaPlaneValue> *field);
void RunSyntheticHeat2DHost(const SyntheticHeat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial,
    std::vector<AlpakaPlaneValue> *result);
void RunSyntheticHeat2DGpu(const SyntheticHeat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial,
    std::vector<AlpakaPlaneValue> *result);
AlpakaPlaneValue MaxAbsDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual);

} // namespace capow

#endif
