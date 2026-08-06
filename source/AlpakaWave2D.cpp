#include "AlpakaWave2D.hpp"

#include "CapowRules.hpp"

#include <alpaka/alpaka.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace
{

using WorkDim = alpaka::DimInt<2U>;
using MemDim = alpaka::DimInt<1U>;
using Idx = std::uint32_t;
using Acc = alpaka::AccGpuCudaRt<WorkDim, Idx>;
using AccPlatform = alpaka::Platform<Acc>;
using HostPlatform = alpaka::PlatformCpu;
using AccDevice = alpaka::Dev<AccPlatform>;
using HostDevice = alpaka::Dev<HostPlatform>;
using Queue = alpaka::Queue<Acc, alpaka::Blocking>;
using MemExtent = alpaka::Vec<MemDim, Idx>;
using WorkExtent = alpaka::Vec<WorkDim, Idx>;
using WorkDiv = alpaka::WorkDivMembers<WorkDim, Idx>;
using HostView = alpaka::ViewPlainPtr<HostDevice, capow::AlpakaPlaneValue, MemDim, Idx>;
using ConstHostView = alpaka::ViewPlainPtr<HostDevice, const capow::AlpakaPlaneValue, MemDim, Idx>;
using DeviceBuffer = alpaka::Buf<AccPlatform, capow::AlpakaPlaneValue, MemDim, Idx>;

const capow::AlpakaPlaneValue defaultWaveSpeed2TimeStep2OverDx2 = capow::AlpakaPlaneValue(0.5F);
const capow::AlpakaPlaneValue defaultMaxIntensity = capow::AlpakaPlaneValue(10.0F);
const capow::AlpakaPlaneValue defaultTimeStep = capow::AlpakaPlaneValue(0.25F);

struct Wave2DKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, const capow::AlpakaPlaneValue *source,
        const capow::AlpakaPlaneValue *past, capow::AlpakaPlaneValue *targetIntensity,
        capow::AlpakaPlaneValue *targetVelocity, Idx width, Idx height,
        capow::AlpakaPlaneValue waveSpeed2TimeStep2OverDx2, capow::AlpakaPlaneValue maxIntensity,
        capow::AlpakaPlaneValue timeStep) const
    {
        const alpaka::Vec<WorkDim, Idx> global = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc);
        const Idx y = global[0];
        const Idx x = global[1];

        if (x >= width || y >= height)
        {
            return;
        }

        const capow::Wave2DResult<capow::AlpakaPlaneValue> result = capow::ComputeWave2DCell<capow::AlpakaPlaneValue>(
            source, past, x, y, width, height, waveSpeed2TimeStep2OverDx2, maxIntensity, timeStep);
        const Idx center = capow::Heat2DIndex(x, y, width);
        targetIntensity[center] = result.nextIntensity;
        targetVelocity[center] = result.velocity;
    }
};

void ValidateOptions(const capow::Wave2DOptions &options)
{
    if (options.width <= 0 || options.height <= 0)
    {
        throw std::invalid_argument("CA_WAVE_2D dimensions must be positive");
    }
    if (options.steps < 0)
    {
        throw std::invalid_argument("CA_WAVE_2D steps must not be negative");
    }
    if (options.maxIntensity <= capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("CA_WAVE_2D max intensity must be positive");
    }
    if (options.timeStep == capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("CA_WAVE_2D time step must not be zero");
    }
}

std::size_t CellCount(const capow::Wave2DOptions &options)
{
    return static_cast<std::size_t>(options.width) * static_cast<std::size_t>(options.height);
}

void ValidateFieldSize(const capow::Wave2DOptions &options, const std::vector<capow::AlpakaPlaneValue> &field)
{
    if (field.size() != CellCount(options))
    {
        throw std::invalid_argument("CA_WAVE_2D field size does not match dimensions");
    }
}

capow::AlpakaPlaneValue UnitValue(std::uint32_t index)
{
    std::uint32_t value = index * 1103515245U + 12345U + 0x9e37U;
    value ^= value >> 15U;
    return static_cast<capow::AlpakaPlaneValue>(value & 0x00ffffffU) /
        static_cast<capow::AlpakaPlaneValue>(0x00ffffffU);
}

void Wave2DStepHost(const capow::Wave2DOptions &options, const std::vector<capow::AlpakaPlaneValue> &source,
    const std::vector<capow::AlpakaPlaneValue> &past, std::vector<capow::AlpakaPlaneValue> *targetIntensity,
    std::vector<capow::AlpakaPlaneValue> *targetVelocity)
{
    const std::uint32_t width = static_cast<std::uint32_t>(options.width);
    const std::uint32_t height = static_cast<std::uint32_t>(options.height);
    for (std::uint32_t y = 0U; y < height; ++y)
    {
        for (std::uint32_t x = 0U; x < width; ++x)
        {
            const capow::Wave2DResult<capow::AlpakaPlaneValue> result =
                capow::ComputeWave2DCell<capow::AlpakaPlaneValue>(source.data(), past.data(), x, y, width, height,
                    options.waveSpeed2TimeStep2OverDx2, options.maxIntensity, options.timeStep);
            const std::uint32_t center = capow::Heat2DIndex(x, y, width);
            (*targetIntensity)[center] = result.nextIntensity;
            (*targetVelocity)[center] = result.velocity;
        }
    }
}

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor)
{
    return (value + divisor - 1U) / divisor;
}

} // namespace

namespace capow
{

Wave2DOptions::Wave2DOptions() :
    width(0),
    height(0),
    steps(0),
    waveSpeed2TimeStep2OverDx2(defaultWaveSpeed2TimeStep2OverDx2),
    maxIntensity(defaultMaxIntensity),
    timeStep(defaultTimeStep)
{
}

void MakeWave2DInitial(
    const Wave2DOptions &options, std::vector<AlpakaPlaneValue> *source, std::vector<AlpakaPlaneValue> *past)
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

void RunWave2DHost(const Wave2DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave2DFields *result)
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
        Wave2DStepHost(options, current, past, &nextIntensity, &nextVelocity);
        std::swap(past, current);
        std::swap(current, nextIntensity);
    }

    result->intensityField = current;
    result->velocityField = nextVelocity;
}

void RunWave2DGpu(const Wave2DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave2DFields *result)
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
    const MemExtent memExtent = MemExtent{static_cast<Idx>(CellCount(options))};

    DeviceBuffer deviceCurrent = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    DeviceBuffer devicePast = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    DeviceBuffer deviceNextIntensity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    DeviceBuffer deviceNextVelocity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    ConstHostView hostSource = alpaka::createView(hostDevice, initialSource.data(), memExtent);
    ConstHostView hostPast = alpaka::createView(hostDevice, initialPast.data(), memExtent);
    alpaka::memcpy(queue, deviceCurrent, hostSource, memExtent);
    alpaka::memcpy(queue, devicePast, hostPast, memExtent);
    alpaka::wait(queue);

    const WorkExtent threads = WorkExtent{16U, 16U};
    const WorkExtent blocks = WorkExtent{DivideRoundUp(static_cast<std::uint32_t>(options.height), threads[0]),
        DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[1])};
    const WorkExtent elements = WorkExtent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const Wave2DKernel kernel = Wave2DKernel{};

    for (int step = 0; step < options.steps; ++step)
    {
        alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(deviceCurrent),
            alpaka::getPtrNative(devicePast), alpaka::getPtrNative(deviceNextIntensity),
            alpaka::getPtrNative(deviceNextVelocity), static_cast<Idx>(options.width), static_cast<Idx>(options.height),
            options.waveSpeed2TimeStep2OverDx2, options.maxIntensity, options.timeStep);
        std::swap(devicePast, deviceCurrent);
        std::swap(deviceCurrent, deviceNextIntensity);
    }
    alpaka::wait(queue);

    result->intensityField.resize(CellCount(options));
    result->velocityField.resize(CellCount(options));
    HostView hostIntensity = alpaka::createView(hostDevice, result->intensityField.data(), memExtent);
    HostView hostVelocity = alpaka::createView(hostDevice, result->velocityField.data(), memExtent);
    alpaka::memcpy(queue, hostIntensity, deviceCurrent, memExtent);
    alpaka::memcpy(queue, hostVelocity, deviceNextVelocity, memExtent);
    alpaka::wait(queue);
}

AlpakaPlaneValue MaxWave2DDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual)
{
    if (expected.size() != actual.size())
    {
        throw std::invalid_argument("CA_WAVE_2D comparison sizes differ");
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
