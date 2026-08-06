#include "AlpakaTiming.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{

std::string FormatDouble(double value)
{
    if (std::fabs(value) < 1.0e-12)
    {
        return "0";
    }

    std::ostringstream output;
    output << std::setprecision(6) << value;
    return output.str();
}

std::string GridText(const capow::AlpakaTimingRow &row)
{
    std::ostringstream output;
    output << row.width << "x" << row.height;
    return output.str();
}

template <typename TValue>
void WriteTimingRow(
    std::ostringstream &output, const char *metric, const std::vector<capow::AlpakaTimingRow> &rows, TValue value)
{
    output << "| " << metric;
    for (std::size_t index = 0U; index < rows.size(); ++index)
    {
        output << " | " << value(rows[index]);
    }
    output << " |\n";
}

} // namespace

namespace capow
{

void AddAlpakaTiming(AlpakaTimingMeasurements *target, AlpakaTimingMetric metric, double milliseconds)
{
    if (target == nullptr)
    {
        return;
    }

    switch (metric)
    {
    case ALPAKA_TIMING_CPU_UPDATE:
        target->cpuUpdateMs += milliseconds;
        break;
    case ALPAKA_TIMING_GPU_UPDATE_ONLY:
        target->gpuUpdateOnlyMs += milliseconds;
        break;
    case ALPAKA_TIMING_GPU_INTEROP:
        target->gpuInteropMs += milliseconds;
        break;
    case ALPAKA_TIMING_GPU_TEXTURE:
        target->gpuTextureMs += milliseconds;
        break;
    case ALPAKA_TIMING_GPU_RENDER:
        target->gpuRenderMs += milliseconds;
        break;
    case ALPAKA_TIMING_GPU_BATCH_SAVE_READBACK:
        target->gpuBatchSaveReadbackMs += milliseconds;
        break;
    }
}

double AlpakaGpuTotalMs(const AlpakaTimingMeasurements &timing)
{
    return timing.gpuUpdateOnlyMs + timing.gpuInteropMs + timing.gpuTextureMs + timing.gpuRenderMs +
        timing.gpuBatchSaveReadbackMs;
}

std::string FormatAlpakaTimingMarkdownTable(const std::vector<AlpakaTimingRow> &rows)
{
    if (rows.empty())
    {
        return "";
    }

    std::ostringstream output;
    output << "| metric";
    for (std::size_t index = 0U; index < rows.size(); ++index)
    {
        output << " | " << rows[index].rule;
    }
    output << " |\n";

    output << "| ---";
    for (std::size_t index = 0U; index < rows.size(); ++index)
    {
        output << " | ---:";
    }
    output << " |\n";

    WriteTimingRow(output, "rule", rows, [](const AlpakaTimingRow &row) { return row.rule; });
    WriteTimingRow(output, "grid", rows, [](const AlpakaTimingRow &row) { return GridText(row); });
    WriteTimingRow(output, "steps", rows, [](const AlpakaTimingRow &row) { return std::to_string(row.steps); });
    WriteTimingRow(
        output, "cpu_update_ms", rows, [](const AlpakaTimingRow &row) { return FormatDouble(row.timing.cpuUpdateMs); });
    WriteTimingRow(output, "gpu_update_only_ms", rows,
        [](const AlpakaTimingRow &row) { return FormatDouble(row.timing.gpuUpdateOnlyMs); });
    WriteTimingRow(output, "gpu_interop_ms", rows,
        [](const AlpakaTimingRow &row) { return FormatDouble(row.timing.gpuInteropMs); });
    WriteTimingRow(output, "gpu_texture_ms", rows,
        [](const AlpakaTimingRow &row) { return FormatDouble(row.timing.gpuTextureMs); });
    WriteTimingRow(
        output, "gpu_render_ms", rows, [](const AlpakaTimingRow &row) { return FormatDouble(row.timing.gpuRenderMs); });
    WriteTimingRow(output, "gpu_batch_save_readback_ms", rows,
        [](const AlpakaTimingRow &row) { return FormatDouble(row.timing.gpuBatchSaveReadbackMs); });
    WriteTimingRow(output, "gpu_total_ms", rows,
        [](const AlpakaTimingRow &row) { return FormatDouble(AlpakaGpuTotalMs(row.timing)); });
    WriteTimingRow(output, "max_error", rows, [](const AlpakaTimingRow &row) { return FormatDouble(row.maxError); });
    return output.str();
}

std::string FormatAlpakaTimingMarkdownDocument(const std::string &title, const std::string &measured,
    const std::string &build, const std::string &scope, const std::vector<AlpakaTimingRow> &rows)
{
    std::ostringstream output;
    output << "# " << title << "\n\n";
    output << "Measured: " << measured << "\n\n";
    output << "Build: `" << build << "`\n\n";
    if (!scope.empty())
    {
        output << "Scope: " << scope << "\n\n";
    }
    output << FormatAlpakaTimingMarkdownTable(rows);
    return output.str();
}

} // namespace capow
