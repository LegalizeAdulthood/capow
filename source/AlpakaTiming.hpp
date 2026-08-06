#ifndef ALPAKATIMING_HPP
#define ALPAKATIMING_HPP

#include <string>
#include <vector>

namespace capow
{

enum AlpakaTimingMetric
{
    ALPAKA_TIMING_CPU_UPDATE,
    ALPAKA_TIMING_GPU_UPDATE_ONLY,
    ALPAKA_TIMING_GPU_INTEROP,
    ALPAKA_TIMING_GPU_TEXTURE,
    ALPAKA_TIMING_GPU_RENDER,
    ALPAKA_TIMING_GPU_BATCH_SAVE_READBACK
};

struct AlpakaTimingMeasurements
{
    double cpuUpdateMs = 0.0;
    double gpuUpdateOnlyMs = 0.0;
    double gpuInteropMs = 0.0;
    double gpuTextureMs = 0.0;
    double gpuRenderMs = 0.0;
    double gpuBatchSaveReadbackMs = 0.0;
};

struct AlpakaTimingRow
{
    std::string rule;
    int width = 0;
    int height = 0;
    int steps = 0;
    AlpakaTimingMeasurements timing;
    double maxError = 0.0;
};

void AddAlpakaTiming(AlpakaTimingMeasurements *target, AlpakaTimingMetric metric, double milliseconds);
double AlpakaGpuTotalMs(const AlpakaTimingMeasurements &timing);
std::string FormatAlpakaTimingMarkdownTable(const std::vector<AlpakaTimingRow> &rows);
std::string FormatAlpakaTimingMarkdownDocument(const std::string &title, const std::string &measured,
    const std::string &build, const std::string &scope, const std::vector<AlpakaTimingRow> &rows);

} // namespace capow

#endif
