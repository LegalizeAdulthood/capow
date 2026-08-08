#ifndef ALPAKADIGITAL_HPP
#define ALPAKADIGITAL_HPP

#include "AlpakaBuffers.hpp"

#include <vector>

namespace capow
{

struct StandardDigitalOptions
{
    int width;
    int steps;
    int stateCount;
    int radius;
    int stateBits;
    int lookupCount;

    StandardDigitalOptions();
};

void MakeStandardDigitalInitial(const StandardDigitalOptions &options, std::vector<AlpakaDigitalValue> *source);
void MakeStandardDigitalLookup(const StandardDigitalOptions &options, std::vector<AlpakaDigitalValue> *lookup);
void RunStandardDigitalHost(const StandardDigitalOptions &options, const std::vector<AlpakaDigitalValue> &initialSource,
    const std::vector<AlpakaDigitalValue> &lookup, std::vector<AlpakaDigitalValue> *result);
void RunStandardDigitalGpu(const StandardDigitalOptions &options, const std::vector<AlpakaDigitalValue> &initialSource,
    const std::vector<AlpakaDigitalValue> &lookup, std::vector<AlpakaDigitalValue> *result);
int CountStandardDigitalDifferences(
    const std::vector<AlpakaDigitalValue> &expected, const std::vector<AlpakaDigitalValue> &actual);
void NormalizeStandardDigitalRow(const StandardDigitalOptions &options, const std::vector<AlpakaDigitalValue> &row,
    std::vector<AlpakaPlaneValue> *normalized);

} // namespace capow

#endif
