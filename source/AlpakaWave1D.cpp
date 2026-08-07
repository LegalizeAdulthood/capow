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
const capow::AlpakaPlaneValue defaultDtOver12Dx2 = capow::AlpakaPlaneValue(0.03125F);
const capow::AlpakaPlaneValue defaultMaxIntensity = capow::AlpakaPlaneValue(10.0F);
const capow::AlpakaPlaneValue defaultMaxVelocity = capow::AlpakaPlaneValue(10.0F);
const capow::AlpakaPlaneValue defaultTimeStep = capow::AlpakaPlaneValue(0.25F);
const capow::AlpakaPlaneValue defaultDtOverMass = capow::AlpakaPlaneValue(0.125F);
const capow::AlpakaPlaneValue defaultFrictionMultiplier = capow::AlpakaPlaneValue(0.25F);
const capow::AlpakaPlaneValue defaultSpringMultiplier = capow::AlpakaPlaneValue(0.75F);
const capow::AlpakaPlaneValue defaultDriverValue = capow::AlpakaPlaneValue(0.5F);

struct Wave1DKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, const capow::AlpakaPlaneValue *source,
        const capow::AlpakaPlaneValue *past, const capow::AlpakaPlaneValue *sourceVelocity,
        const capow::AlpakaPlaneValue *frictionTweaks, const capow::AlpakaPlaneValue *springTweaks,
        const capow::AlpakaPlaneValue *massTweaks, capow::AlpakaPlaneValue *targetIntensity,
        capow::AlpakaPlaneValue *targetVelocity, Idx width, capow::AlpakaPlaneValue waveSpeed2TimeStep2OverDx2,
        capow::AlpakaPlaneValue dtOver12Dx2, capow::AlpakaPlaneValue maxIntensity, capow::AlpakaPlaneValue maxVelocity,
        capow::AlpakaPlaneValue timeStep, capow::AlpakaPlaneValue dtOverMass,
        capow::AlpakaPlaneValue frictionMultiplier, capow::AlpakaPlaneValue springMultiplier,
        capow::AlpakaPlaneValue driverValue, capow::Wave1DRule rule) const
    {
        const Idx x = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0];
        if (x >= width)
        {
            return;
        }

        capow::Wave1DResult<capow::AlpakaPlaneValue> result = {capow::AlpakaPlaneValue(0), capow::AlpakaPlaneValue(0)};
        if (rule == capow::WAVE_1D_RULE_OSCILLATOR)
        {
            result = capow::ComputeOscillator1D<capow::AlpakaPlaneValue>(source[x], sourceVelocity[x], dtOverMass,
                frictionMultiplier, springMultiplier, driverValue, maxIntensity, maxVelocity, timeStep);
        }
        else if (rule == capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR)
        {
            result = capow::ComputeDiverseOscillator1D<capow::AlpakaPlaneValue>(source[x], sourceVelocity[x],
                dtOverMass, frictionMultiplier, springMultiplier, driverValue, frictionTweaks[x], springTweaks[x],
                massTweaks[x], maxIntensity, maxVelocity, timeStep);
        }
        else if (rule == capow::WAVE_1D_RULE_FIVE_NEIGHBOR)
        {
            result = capow::ComputeWave1D5Cell<capow::AlpakaPlaneValue>(
                source, past, x, width, dtOver12Dx2, maxIntensity, maxVelocity, timeStep);
        }
        else
        {
            result = capow::ComputeWave1DCell<capow::AlpakaPlaneValue>(
                source, past, x, width, waveSpeed2TimeStep2OverDx2, maxIntensity, timeStep);
        }
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
    if (options.maxVelocity <= capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("CA_WAVE max velocity must be positive");
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

bool IsOscillatorRule(capow::Wave1DRule rule)
{
    return rule == capow::WAVE_1D_RULE_OSCILLATOR || rule == capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR;
}

void MakeWave1DVelocityFromPast(const capow::Wave1DOptions &options, const std::vector<capow::AlpakaPlaneValue> &source,
    const std::vector<capow::AlpakaPlaneValue> &past, std::vector<capow::AlpakaPlaneValue> *targetVelocity)
{
    targetVelocity->resize(CellCount(options));
    for (std::size_t index = 0; index < targetVelocity->size(); ++index)
    {
        (*targetVelocity)[index] = (source[index] - past[index]) / options.timeStep;
    }
}

void MakeWave1DTweaks(const capow::Wave1DOptions &options, std::vector<capow::AlpakaPlaneValue> *frictionTweaks,
    std::vector<capow::AlpakaPlaneValue> *springTweaks, std::vector<capow::AlpakaPlaneValue> *massTweaks)
{
    const std::size_t width = CellCount(options);
    frictionTweaks->resize(width);
    springTweaks->resize(width);
    massTweaks->resize(width);
    for (std::size_t index = 0; index < width; ++index)
    {
        (*frictionTweaks)[index] = capow::AlpakaPlaneValue(0.8F) +
            capow::AlpakaPlaneValue(0.4F) * UnitValue(static_cast<std::uint32_t>(index + width));
        (*springTweaks)[index] = capow::AlpakaPlaneValue(0.75F) +
            capow::AlpakaPlaneValue(0.5F) * UnitValue(static_cast<std::uint32_t>(index + 2U * width));
        (*massTweaks)[index] = capow::AlpakaPlaneValue(0.8F) +
            capow::AlpakaPlaneValue(0.4F) * UnitValue(static_cast<std::uint32_t>(index + 3U * width));
    }
}

void Wave1DStepHost(const capow::Wave1DOptions &options, const std::vector<capow::AlpakaPlaneValue> &source,
    const std::vector<capow::AlpakaPlaneValue> &past, const std::vector<capow::AlpakaPlaneValue> &sourceVelocity,
    const std::vector<capow::AlpakaPlaneValue> &frictionTweaks,
    const std::vector<capow::AlpakaPlaneValue> &springTweaks, const std::vector<capow::AlpakaPlaneValue> &massTweaks,
    std::vector<capow::AlpakaPlaneValue> *targetIntensity, std::vector<capow::AlpakaPlaneValue> *targetVelocity)
{
    const std::uint32_t width = static_cast<std::uint32_t>(options.width);
    for (std::uint32_t x = 0U; x < width; ++x)
    {
        capow::Wave1DResult<capow::AlpakaPlaneValue> result = {capow::AlpakaPlaneValue(0), capow::AlpakaPlaneValue(0)};
        if (options.rule == capow::WAVE_1D_RULE_OSCILLATOR)
        {
            result = capow::ComputeOscillator1D<capow::AlpakaPlaneValue>(source[x], sourceVelocity[x],
                options.dtOverMass, options.frictionMultiplier, options.springMultiplier, options.driverValue,
                options.maxIntensity, options.maxVelocity, options.timeStep);
        }
        else if (options.rule == capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR)
        {
            result = capow::ComputeDiverseOscillator1D<capow::AlpakaPlaneValue>(source[x], sourceVelocity[x],
                options.dtOverMass, options.frictionMultiplier, options.springMultiplier, options.driverValue,
                frictionTweaks[x], springTweaks[x], massTweaks[x], options.maxIntensity, options.maxVelocity,
                options.timeStep);
        }
        else if (options.rule == capow::WAVE_1D_RULE_FIVE_NEIGHBOR)
        {
            result = capow::ComputeWave1D5Cell<capow::AlpakaPlaneValue>(source.data(), past.data(), x, width,
                options.dtOver12Dx2, options.maxIntensity, options.maxVelocity, options.timeStep);
        }
        else
        {
            result = capow::ComputeWave1DCell<capow::AlpakaPlaneValue>(source.data(), past.data(), x, width,
                options.waveSpeed2TimeStep2OverDx2, options.maxIntensity, options.timeStep);
        }
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
    rule(WAVE_1D_RULE_THREE_NEIGHBOR),
    waveSpeed2TimeStep2OverDx2(defaultWaveSpeed2TimeStep2OverDx2),
    dtOver12Dx2(defaultDtOver12Dx2),
    maxIntensity(defaultMaxIntensity),
    maxVelocity(defaultMaxVelocity),
    timeStep(defaultTimeStep),
    dtOverMass(defaultDtOverMass),
    frictionMultiplier(defaultFrictionMultiplier),
    springMultiplier(defaultSpringMultiplier),
    driverValue(defaultDriverValue)
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
        (*source)[index] =
            (sourceUnit * AlpakaPlaneValue(2) - AlpakaPlaneValue(1)) * options.maxIntensity * AlpakaPlaneValue(0.5F);
        if (IsOscillatorRule(options.rule))
        {
            const AlpakaPlaneValue velocityUnit = UnitValue(static_cast<std::uint32_t>(index + source->size()));
            const AlpakaPlaneValue velocity = (velocityUnit * AlpakaPlaneValue(2) - AlpakaPlaneValue(1)) *
                options.maxVelocity * AlpakaPlaneValue(0.25F);
            (*past)[index] = (*source)[index] - velocity * options.timeStep;
        }
        else
        {
            const AlpakaPlaneValue pastUnit = UnitValue(static_cast<std::uint32_t>(index + source->size()));
            (*past)[index] =
                (pastUnit * AlpakaPlaneValue(2) - AlpakaPlaneValue(1)) * options.maxIntensity * AlpakaPlaneValue(0.5F);
        }
    }
}

void RunWave1DHost(const Wave1DOptions &options, const std::vector<AlpakaPlaneValue> &initialSource,
    const std::vector<AlpakaPlaneValue> &initialPast, Wave1DFields *result)
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

    std::vector<AlpakaPlaneValue> current = initialSource;
    std::vector<AlpakaPlaneValue> past = initialPast;
    std::vector<AlpakaPlaneValue> currentVelocity;
    std::vector<AlpakaPlaneValue> nextIntensity(CellCount(options));
    std::vector<AlpakaPlaneValue> nextVelocity(CellCount(options), AlpakaPlaneValue(0));
    std::vector<AlpakaPlaneValue> frictionTweaks;
    std::vector<AlpakaPlaneValue> springTweaks;
    std::vector<AlpakaPlaneValue> massTweaks;
    MakeWave1DVelocityFromPast(options, current, past, &currentVelocity);
    MakeWave1DTweaks(options, &frictionTweaks, &springTweaks, &massTweaks);
    for (int step = 0; step < options.steps; ++step)
    {
        Wave1DStepHost(options, current, past, currentVelocity, frictionTweaks, springTweaks, massTweaks,
            &nextIntensity, &nextVelocity);
        std::swap(past, current);
        std::swap(current, nextIntensity);
        std::swap(currentVelocity, nextVelocity);
    }

    result->intensityField = current;
    result->velocityField = currentVelocity;
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
    std::vector<AlpakaPlaneValue> initialVelocity;
    std::vector<AlpakaPlaneValue> frictionTweaks;
    std::vector<AlpakaPlaneValue> springTweaks;
    std::vector<AlpakaPlaneValue> massTweaks;
    MakeWave1DVelocityFromPast(options, initialSource, initialPast, &initialVelocity);
    MakeWave1DTweaks(options, &frictionTweaks, &springTweaks, &massTweaks);

    DeviceBuffer deviceCurrent = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer devicePast = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceNextIntensity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceCurrentVelocity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceNextVelocity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceFrictionTweaks = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceSpringTweaks = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    DeviceBuffer deviceMassTweaks = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent);
    ConstHostView hostSource = alpaka::createView(hostDevice, initialSource.data(), extent);
    ConstHostView hostPast = alpaka::createView(hostDevice, initialPast.data(), extent);
    ConstHostView hostInitialVelocity =
        alpaka::createView(hostDevice, static_cast<const AlpakaPlaneValue *>(initialVelocity.data()), extent);
    ConstHostView hostFrictionTweaks =
        alpaka::createView(hostDevice, static_cast<const AlpakaPlaneValue *>(frictionTweaks.data()), extent);
    ConstHostView hostSpringTweaks =
        alpaka::createView(hostDevice, static_cast<const AlpakaPlaneValue *>(springTweaks.data()), extent);
    ConstHostView hostMassTweaks =
        alpaka::createView(hostDevice, static_cast<const AlpakaPlaneValue *>(massTweaks.data()), extent);
    alpaka::memcpy(queue, deviceCurrent, hostSource, extent);
    alpaka::memcpy(queue, devicePast, hostPast, extent);
    alpaka::memcpy(queue, deviceCurrentVelocity, hostInitialVelocity, extent);
    alpaka::memcpy(queue, deviceFrictionTweaks, hostFrictionTweaks, extent);
    alpaka::memcpy(queue, deviceSpringTweaks, hostSpringTweaks, extent);
    alpaka::memcpy(queue, deviceMassTweaks, hostMassTweaks, extent);
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
                    alpaka::getPtrNative(devicePast), alpaka::getPtrNative(deviceCurrentVelocity),
                    alpaka::getPtrNative(deviceFrictionTweaks), alpaka::getPtrNative(deviceSpringTweaks),
                    alpaka::getPtrNative(deviceMassTweaks), alpaka::getPtrNative(deviceNextIntensity),
                    alpaka::getPtrNative(deviceNextVelocity), static_cast<Idx>(options.width),
                    options.waveSpeed2TimeStep2OverDx2, options.dtOver12Dx2, options.maxIntensity, options.maxVelocity,
                    options.timeStep, options.dtOverMass, options.frictionMultiplier, options.springMultiplier,
                    options.driverValue, options.rule);
                std::swap(devicePast, deviceCurrent);
                std::swap(deviceCurrent, deviceNextIntensity);
                std::swap(deviceCurrentVelocity, deviceNextVelocity);
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
            alpaka::memcpy(queue, hostVelocity, deviceCurrentVelocity, extent);
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
