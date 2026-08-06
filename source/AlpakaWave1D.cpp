#include "AlpakaWave1D.hpp"

#include "CapowRules.hpp"

#include <alpaka/alpaka.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace
{

using WorkDim = alpaka::DimInt<1U>;
using MemDim = alpaka::DimInt<1U>;
using Idx = std::uint32_t;
using Acc = alpaka::AccGpuCudaRt<WorkDim, Idx>;
using AccPlatform = alpaka::Platform<Acc>;
using HostPlatform = alpaka::PlatformCpu;
using AccDevice = alpaka::Dev<AccPlatform>;
using HostDevice = alpaka::Dev<HostPlatform>;
using Queue = alpaka::Queue<Acc, alpaka::Blocking>;
using Extent = alpaka::Vec<MemDim, Idx>;
using WorkDiv = alpaka::WorkDivMembers<WorkDim, Idx>;
using HostView = alpaka::ViewPlainPtr<HostDevice, capow::AlpakaPlaneValue, MemDim, Idx>;
using ConstHostView = alpaka::ViewPlainPtr<HostDevice, const capow::AlpakaPlaneValue, MemDim, Idx>;
using DeviceBuffer = alpaka::Buf<AccPlatform, capow::AlpakaPlaneValue, MemDim, Idx>;

const capow::AlpakaPlaneValue defaultWaveSpeed2TimeStep2OverDx2 = capow::AlpakaPlaneValue(0.5F);
const capow::AlpakaPlaneValue defaultMaxIntensity = capow::AlpakaPlaneValue(10.0F);
const capow::AlpakaPlaneValue defaultTimeStep = capow::AlpakaPlaneValue(0.25F);

struct Wave1DKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, const capow::AlpakaPlaneValue *source,
        const capow::AlpakaPlaneValue *past, capow::AlpakaPlaneValue *targetIntensity,
        capow::AlpakaPlaneValue *targetVelocity, Idx width, capow::AlpakaPlaneValue waveSpeed2TimeStep2OverDx2,
        capow::AlpakaPlaneValue maxIntensity, capow::AlpakaPlaneValue timeStep) const
    {
        const Idx x = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0];
        if (x >= width)
        {
            return;
        }

        const capow::Wave1DResult<capow::AlpakaPlaneValue> result = capow::ComputeWave1DCell<capow::AlpakaPlaneValue>(
            source, past, x, width, waveSpeed2TimeStep2OverDx2, maxIntensity, timeStep);
        targetIntensity[x] = result.nextIntensity;
        targetVelocity[x] = result.velocity;
    }
};

void ValidateOptions(const capow::Wave1DOptions &options)
{
    if (options.width <= 0)
    {
        throw std::invalid_argument("CA_WAVE width must be positive");
    }
    if (options.steps < 0)
    {
        throw std::invalid_argument("CA_WAVE steps must not be negative");
    }
    if (options.maxIntensity <= capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("CA_WAVE max intensity must be positive");
    }
    if (options.timeStep == capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("CA_WAVE time step must not be zero");
    }
}

std::size_t CellCount(const capow::Wave1DOptions &options)
{
    return static_cast<std::size_t>(options.width);
}

void ValidateFieldSize(const capow::Wave1DOptions &options, const std::vector<capow::AlpakaPlaneValue> &field)
{
    if (field.size() != CellCount(options))
    {
        throw std::invalid_argument("CA_WAVE field size does not match width");
    }
}

capow::AlpakaPlaneValue UnitValue(std::uint32_t index)
{
    std::uint32_t value = index * 1103515245U + 12345U + 0x5eedU;
    value ^= value >> 15U;
    return static_cast<capow::AlpakaPlaneValue>(value & 0x00ffffffU) /
        static_cast<capow::AlpakaPlaneValue>(0x00ffffffU);
}

void Wave1DStepHost(const capow::Wave1DOptions &options, const std::vector<capow::AlpakaPlaneValue> &source,
    const std::vector<capow::AlpakaPlaneValue> &past, std::vector<capow::AlpakaPlaneValue> *targetIntensity,
    std::vector<capow::AlpakaPlaneValue> *targetVelocity)
{
    const std::uint32_t width = static_cast<std::uint32_t>(options.width);
    for (std::uint32_t x = 0U; x < width; ++x)
    {
        const capow::Wave1DResult<capow::AlpakaPlaneValue> result =
            capow::ComputeWave1DCell<capow::AlpakaPlaneValue>(source.data(), past.data(), x, width,
                options.waveSpeed2TimeStep2OverDx2, options.maxIntensity, options.timeStep);
        (*targetIntensity)[x] = result.nextIntensity;
        (*targetVelocity)[x] = result.velocity;
    }
}

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor)
{
    return (value + divisor - 1U) / divisor;
}

template <typename TFunc>
double MeasureMilliseconds(TFunc function)
{
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    function();
    const std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(finish - start).count();
}

} // namespace

namespace capow
{

Wave1DOptions::Wave1DOptions() :
    width(0),
    steps(0),
    waveSpeed2TimeStep2OverDx2(defaultWaveSpeed2TimeStep2OverDx2),
    maxIntensity(defaultMaxIntensity),
    timeStep(defaultTimeStep)
{
}

void MakeWave1DInitial(
    const Wave1DOptions &options, std::vector<AlpakaPlaneValue> *source, std::vector<AlpakaPlaneValue> *past)
{
    ValidateOptions(options);
    source->resize(CellCount(options));
    past->resize(CellCount(options));
    for (std::size_t index = 0; index < source->size(); ++index)
    {
        const AlpakaPlaneValue sourceUnit = UnitValue(static_cast<std::uint32_t>(index));
        const AlpakaPlaneValue pastUnit = UnitValue(static_cast<std::uint32_t>(index + source->size()));
        (*source)[index] =
            (sourceUnit * AlpakaPlaneValue(2) - AlpakaPlaneValue(1)) * options.maxIntensity * AlpakaPlaneValue(0.5F);
        (*past)[index] =
            (pastUnit * AlpakaPlaneValue(2) - AlpakaPlaneValue(1)) * options.maxIntensity * AlpakaPlaneValue(0.5F);
    }
}

void RunWave1DHost(const Wave1DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave1DFields *result)
{
    ValidateOptions(options);
    ValidateFieldSize(options, initialSource);
    ValidateFieldSize(options, initialPast);

    std::vector<AlpakaPlaneValue> current = initialSource;
    std::vector<AlpakaPlaneValue> past = initialPast;
    std::vector<AlpakaPlaneValue> nextIntensity(CellCount(options));
    std::vector<AlpakaPlaneValue> nextVelocity(CellCount(options), AlpakaPlaneValue(0));
    for (int step = 0; step < options.steps; ++step)
    {
        Wave1DStepHost(options, current, past, &nextIntensity, &nextVelocity);
        std::swap(past, current);
        std::swap(current, nextIntensity);
    }

    result->intensityField = current;
    result->velocityField = nextVelocity;
}

void RunWave1DGpu(const Wave1DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave1DFields *result)
{
    RunWave1DGpuTimed(options, initialSource, initialPast, result, nullptr);
}

void RunWave1DGpuTimed(const Wave1DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave1DFields *result, AlpakaTimingMeasurements *timing)
{
    ValidateOptions(options);
    ValidateFieldSize(options, initialSource);
    ValidateFieldSize(options, initialPast);

    if (options.steps == 0)
    {
        result->intensityField = initialSource;
        result->velocityField.assign(initialSource.size(), AlpakaPlaneValue(0));
        return;
    }

    const AccPlatform accPlatform = AccPlatform{};
    const HostPlatform hostPlatform = HostPlatform{};
    const AccDevice accDevice = alpaka::getDevByIdx(accPlatform, 0U);
    const HostDevice hostDevice = alpaka::getDevByIdx(hostPlatform, 0U);
    Queue queue(accDevice);
    const Extent extent = Extent{static_cast<Idx>(CellCount(options))};

    DeviceBuffer deviceCurrent = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer devicePast = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceNextIntensity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceNextVelocity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    ConstHostView hostSource = alpaka::createView(hostDevice, initialSource.data(), extent);
    ConstHostView hostPast = alpaka::createView(hostDevice, initialPast.data(), extent);
    alpaka::memcpy(queue, deviceCurrent, hostSource, extent);
    alpaka::memcpy(queue, devicePast, hostPast, extent);
    alpaka::wait(queue);

    const Extent threads = Extent{128U};
    const Extent blocks = Extent{DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[0])};
    const Extent elements = Extent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const Wave1DKernel kernel = Wave1DKernel{};

    const double updateMs = MeasureMilliseconds(
        [&]()
        {
            for (int step = 0; step < options.steps; ++step)
            {
                alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(deviceCurrent),
                    alpaka::getPtrNative(devicePast), alpaka::getPtrNative(deviceNextIntensity),
                    alpaka::getPtrNative(deviceNextVelocity), static_cast<Idx>(options.width),
                    options.waveSpeed2TimeStep2OverDx2, options.maxIntensity, options.timeStep);
                std::swap(devicePast, deviceCurrent);
                std::swap(deviceCurrent, deviceNextIntensity);
            }
            alpaka::wait(queue);
        });
    AddAlpakaTiming(timing, ALPAKA_TIMING_GPU_UPDATE_ONLY, updateMs);

    result->intensityField.resize(CellCount(options));
    result->velocityField.resize(CellCount(options));
    HostView hostIntensity = alpaka::createView(hostDevice, result->intensityField.data(), extent);
    HostView hostVelocity = alpaka::createView(hostDevice, result->velocityField.data(), extent);
    const double readbackMs = MeasureMilliseconds(
        [&]()
        {
            alpaka::memcpy(queue, hostIntensity, deviceCurrent, extent);
            alpaka::memcpy(queue, hostVelocity, deviceNextVelocity, extent);
            alpaka::wait(queue);
        });
    AddAlpakaTiming(timing, ALPAKA_TIMING_GPU_BATCH_SAVE_READBACK, readbackMs);
}

AlpakaPlaneValue MaxWave1DDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual)
{
    if (expected.size() != actual.size())
    {
        throw std::invalid_argument("CA_WAVE comparison sizes differ");
    }

    AlpakaPlaneValue result = AlpakaPlaneValue(0);
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        const AlpakaPlaneValue error = std::fabs(expected[index] - actual[index]);
        result = std::max(result, error);
    }
    return result;
}

} // namespace capow
