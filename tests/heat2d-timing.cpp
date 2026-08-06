#include "AlpakaBackend.hpp"
#include "AlpakaHeat2D.hpp"
#include "AlpakaTiming.hpp"
#include "AlpakaWave2D.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

capow::Heat2DOptions MakeHeatOptions()
{
    capow::Heat2DOptions options;
    options.width = 500;
    options.height = 250;
    options.steps = 1000;
    options.heatIncrement = 0.375F;
    options.maxIntensity = 7.0F;
    options.timeStep = 0.25F;
    return options;
}

capow::Wave2DOptions MakeWaveOptions()
{
    capow::Wave2DOptions options;
    options.width = 500;
    options.height = 250;
    options.steps = 1000;
    options.waveSpeed2TimeStep2OverDx2 = 0.5F;
    options.maxIntensity = 7.0F;
    options.timeStep = 0.25F;
    return options;
}

template <typename TFunc>
double MeasureMilliseconds(TFunc function)
{
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    function();
    const std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

std::string LocalDate()
{
    const std::time_t now = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &now);

    std::ostringstream output;
    output << std::put_time(&localTime, "%Y-%m-%d");
    return output.str();
}

std::string BuildLabel()
{
#if defined(NDEBUG)
    return "heat2d-timing Release";
#else
    return "heat2d-timing Debug";
#endif
}

capow::AlpakaTimingRow MeasureHeat2D(const capow::Heat2DOptions &options)
{
    std::vector<capow::AlpakaPlaneValue> initial;
    capow::Heat2DFields hostResult;
    capow::Heat2DFields gpuResult;
    capow::MakeHeat2DInitial(options, &initial);

    const double cpuMs = MeasureMilliseconds(
        [&options, &initial, &hostResult]() { capow::RunHeat2DHost(options, initial, &hostResult); });

    capow::AlpakaTimingRow row;
    row.rule = "CA_HEAT_2D";
    row.width = options.width;
    row.height = options.height;
    row.steps = options.steps;
    capow::AddAlpakaTiming(&row.timing, capow::ALPAKA_TIMING_CPU_UPDATE, cpuMs);
    capow::RunHeat2DGpuTimed(options, initial, &gpuResult, &row.timing);
    row.maxError =
        std::max(static_cast<double>(capow::MaxHeat2DDifference(hostResult.intensityField, gpuResult.intensityField)),
            static_cast<double>(capow::MaxHeat2DDifference(hostResult.velocityField, gpuResult.velocityField)));
    return row;
}

capow::AlpakaTimingRow MeasureWave2D(const capow::Wave2DOptions &options)
{
    std::vector<capow::AlpakaPlaneValue> source;
    std::vector<capow::AlpakaPlaneValue> past;
    capow::Wave2DFields hostResult;
    capow::Wave2DFields gpuResult;
    capow::MakeWave2DInitial(options, &source, &past);

    const double cpuMs = MeasureMilliseconds(
        [&options, &source, &past, &hostResult]() { capow::RunWave2DHost(options, source, past, &hostResult); });

    capow::AlpakaTimingRow row;
    row.rule = "CA_WAVE_2D";
    row.width = options.width;
    row.height = options.height;
    row.steps = options.steps;
    capow::AddAlpakaTiming(&row.timing, capow::ALPAKA_TIMING_CPU_UPDATE, cpuMs);
    capow::RunWave2DGpuTimed(options, source, past, &gpuResult, &row.timing);
    row.maxError =
        std::max(static_cast<double>(capow::MaxWave2DDifference(hostResult.intensityField, gpuResult.intensityField)),
            static_cast<double>(capow::MaxWave2DDifference(hostResult.velocityField, gpuResult.velocityField)));
    return row;
}

bool WriteMarkdown(const std::string &path, const std::vector<capow::AlpakaTimingRow> &rows)
{
    std::ofstream output(path.c_str());
    if (!output)
    {
        return false;
    }

    const std::string scope = "Host update and GPU harness calls.  Interop, texture, and render\n"
                              "fields are zero until live-app samples are collected.";
    output << capow::FormatAlpakaTimingMarkdownDocument("CAPOW Alpaka Timing", LocalDate(), BuildLabel(), scope, rows);
    return true;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: heat2d-timing output.md\n";
        return 2;
    }

    capow::AlpakaManager &manager = capow::GetAlpakaManager();
    if (!manager.IsGpuAvailable())
    {
        std::cerr << manager.GetAvailabilityMessage() << "\n";
        return 1;
    }

    std::vector<capow::AlpakaTimingRow> rows;
    rows.push_back(MeasureHeat2D(MakeHeatOptions()));
    rows.push_back(MeasureWave2D(MakeWaveOptions()));
    if (!WriteMarkdown(argv[1], rows))
    {
        std::cerr << "could not write " << argv[1] << "\n";
        return 3;
    }

    return 0;
}
