#include "AlpakaBackend.hpp"
#include "AlpakaHeat2D.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{

struct Measurement
{
    double cpuMs;
    double gpuMs;
    capow::AlpakaPlaneValue maxIntensityError;
    capow::AlpakaPlaneValue maxVelocityError;
};

capow::Heat2DOptions MakeOptions()
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

template <typename TFunc>
double MeasureMilliseconds(TFunc function)
{
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    function();
    const std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

Measurement MeasureHeat2D(const capow::Heat2DOptions &options)
{
    std::vector<capow::AlpakaPlaneValue> initial;
    capow::Heat2DFields hostResult;
    capow::Heat2DFields gpuResult;
    capow::MakeHeat2DInitial(options, &initial);

    const double cpuMs = MeasureMilliseconds(
        [&options, &initial, &hostResult]() { capow::RunHeat2DHost(options, initial, &hostResult); });
    const double gpuMs =
        MeasureMilliseconds([&options, &initial, &gpuResult]() { capow::RunHeat2DGpu(options, initial, &gpuResult); });

    Measurement measurement;
    measurement.cpuMs = cpuMs;
    measurement.gpuMs = gpuMs;
    measurement.maxIntensityError = capow::MaxHeat2DDifference(hostResult.intensityField, gpuResult.intensityField);
    measurement.maxVelocityError = capow::MaxHeat2DDifference(hostResult.velocityField, gpuResult.velocityField);
    return measurement;
}

bool WriteMarkdown(const std::string &path, const capow::Heat2DOptions &options, const Measurement &measurement)
{
    std::ofstream output(path.c_str());
    if (!output)
    {
        return false;
    }

    output << "# CA_HEAT_2D Harness Timing\n\n";
    output << "Measured: 2026-08-02\n\n";
    output << "Build: `temp` Release\n\n";
    output << "Scope: host update and GPU harness calls.  GPU time includes\n";
    output << "host/device transfers used by the current harness.\n\n";
    output << "| metric | value |\n";
    output << "| --- | ---: |\n";
    output << "| rule | CA_HEAT_2D |\n";
    output << "| grid | " << options.width << "x" << options.height << " |\n";
    output << "| steps | " << options.steps << " |\n";
    output << "| cpu_update_ms | " << measurement.cpuMs << " |\n";
    output << "| gpu_total_ms | " << measurement.gpuMs << " |\n";
    output << "| max_intensity_error | " << measurement.maxIntensityError << " |\n";
    output << "| max_velocity_error | " << measurement.maxVelocityError << " |\n";
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

    const capow::Heat2DOptions options = MakeOptions();
    const Measurement measurement = MeasureHeat2D(options);
    if (!WriteMarkdown(argv[1], options, measurement))
    {
        std::cerr << "could not write " << argv[1] << "\n";
        return 3;
    }

    return 0;
}
