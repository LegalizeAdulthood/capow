#include "AlpakaHeat2D.hpp"

#include "AlpakaUtilities.hpp"
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
using DeviceBuffer = alpaka::Buf<AccPlatform, capow::AlpakaPlaneValue, MemDim, Idx>;

const capow::AlpakaPlaneValue defaultHeatIncrement = capow::AlpakaPlaneValue(0.125F);
const capow::AlpakaPlaneValue defaultMaxIntensity = capow::AlpakaPlaneValue(10.0F);
const capow::AlpakaPlaneValue defaultTimeStep = capow::AlpakaPlaneValue(0.25F);

struct Heat2DKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, const capow::AlpakaPlaneValue *source,
        capow::AlpakaPlaneValue *targetIntensity, capow::AlpakaPlaneValue *targetVelocity, Idx width, Idx height,
        capow::AlpakaPlaneValue heatIncrement, capow::AlpakaPlaneValue maxIntensity,
        capow::AlpakaPlaneValue timeStep) const
    {
        const alpaka::Vec<WorkDim, Idx> global = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc);
        const Idx y = global[0];
        const Idx x = global[1];

        if (x >= width || y >= height)
        {
            return;
        }

        const capow::alpaka_util::FiveNeighborIndexes indexes =
            capow::alpaka_util::five_neighbor_indexes(x, y, width, height);
        const capow::Heat2DResult<capow::AlpakaPlaneValue> result = capow::ComputeHeat2D<capow::AlpakaPlaneValue>(
            source[indexes.center], source[indexes.east], source[indexes.north], source[indexes.west],
            source[indexes.south], heatIncrement, maxIntensity, timeStep);
        targetIntensity[indexes.center] = result.nextIntensity;
        targetVelocity[indexes.center] = result.velocity;
    }
};

void ValidateOptions(const capow::Heat2DOptions &options)
{
    if (options.width <= 0 || options.height <= 0)
    {
        throw std::invalid_argument("CA_HEAT_2D dimensions must be positive");
    }
    if (options.steps < 0)
    {
        throw std::invalid_argument("CA_HEAT_2D steps must not be negative");
    }
    if (options.maxIntensity <= capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("CA_HEAT_2D max intensity must be positive");
    }
    if (options.timeStep == capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("CA_HEAT_2D time step must not be zero");
    }
}

std::size_t CellCount(const capow::Heat2DOptions &options)
{
    return static_cast<std::size_t>(options.width) * static_cast<std::size_t>(options.height);
}

void ValidateFieldSize(const capow::Heat2DOptions &options, const std::vector<capow::AlpakaPlaneValue> &field)
{
    if (field.size() != CellCount(options))
    {
        throw std::invalid_argument("CA_HEAT_2D field size does not match dimensions");
    }
}

capow::AlpakaPlaneValue UnitValue(std::uint32_t index)
{
    std::uint32_t value = index * 1103515245U + 12345U + 0x5678U;
    value ^= value >> 15U;
    return static_cast<capow::AlpakaPlaneValue>(value & 0x00ffffffU) /
        static_cast<capow::AlpakaPlaneValue>(0x00ffffffU);
}

void Heat2DStepHost(const capow::Heat2DOptions &options, const std::vector<capow::AlpakaPlaneValue> &source,
    std::vector<capow::AlpakaPlaneValue> *targetIntensity, std::vector<capow::AlpakaPlaneValue> *targetVelocity)
{
    const std::uint32_t width = static_cast<std::uint32_t>(options.width);
    const std::uint32_t height = static_cast<std::uint32_t>(options.height);
    for (std::uint32_t y = 0U; y < height; ++y)
    {
        for (std::uint32_t x = 0U; x < width; ++x)
        {
            const capow::alpaka_util::FiveNeighborIndexes indexes =
                capow::alpaka_util::five_neighbor_indexes(x, y, width, height);
            const capow::Heat2DResult<capow::AlpakaPlaneValue> result = capow::ComputeHeat2D<capow::AlpakaPlaneValue>(
                source[indexes.center], source[indexes.east], source[indexes.north], source[indexes.west],
                source[indexes.south], options.heatIncrement, options.maxIntensity, options.timeStep);
            (*targetIntensity)[indexes.center] = result.nextIntensity;
            (*targetVelocity)[indexes.center] = result.velocity;
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

Heat2DOptions::Heat2DOptions() :
    width(0),
    height(0),
    steps(0),
    heatIncrement(defaultHeatIncrement),
    maxIntensity(defaultMaxIntensity),
    timeStep(defaultTimeStep)
{
}

void MakeHeat2DInitial(const Heat2DOptions &options, std::vector<AlpakaPlaneValue> *field)
{
    ValidateOptions(options);
    field->resize(CellCount(options));
    for (std::size_t index = 0; index < field->size(); ++index)
    {
        const AlpakaPlaneValue unitValue = UnitValue(static_cast<std::uint32_t>(index));
        (*field)[index] = (unitValue * AlpakaPlaneValue(2) - AlpakaPlaneValue(1)) * options.maxIntensity;
    }
}

void RunHeat2DHost(const Heat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial, Heat2DFields *result)
{
    ValidateOptions(options);
    ValidateFieldSize(options, initial);

    std::vector<AlpakaPlaneValue> current = initial;
    std::vector<AlpakaPlaneValue> nextIntensity(CellCount(options));
    std::vector<AlpakaPlaneValue> nextVelocity(CellCount(options), AlpakaPlaneValue(0));
    for (int step = 0; step < options.steps; ++step)
    {
        Heat2DStepHost(options, current, &nextIntensity, &nextVelocity);
        std::swap(current, nextIntensity);
    }

    result->intensity = current;
    result->velocity = nextVelocity;
}

void RunHeat2DGpu(const Heat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial, Heat2DFields *result)
{
    ValidateOptions(options);
    ValidateFieldSize(options, initial);

    if (options.steps == 0)
    {
        result->intensity = initial;
        result->velocity.assign(initial.size(), AlpakaPlaneValue(0));
        return;
    }

    const AccPlatform accPlatform = AccPlatform{};
    const HostPlatform hostPlatform = HostPlatform{};
    const AccDevice accDevice = alpaka::getDevByIdx(accPlatform, 0U);
    const HostDevice hostDevice = alpaka::getDevByIdx(hostPlatform, 0U);
    Queue queue(accDevice);
    const MemExtent memExtent = MemExtent{static_cast<Idx>(CellCount(options))};

    DeviceBuffer deviceCurrent = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    DeviceBuffer deviceNextIntensity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    DeviceBuffer deviceNextVelocity = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    std::vector<AlpakaPlaneValue> hostInitialData = initial;
    HostView hostInitial = alpaka::createView(hostDevice, hostInitialData.data(), memExtent);
    alpaka::memcpy(queue, deviceCurrent, hostInitial, memExtent);
    alpaka::wait(queue);

    const WorkExtent threads = WorkExtent{16U, 16U};
    const WorkExtent blocks = WorkExtent{DivideRoundUp(static_cast<std::uint32_t>(options.height), threads[0]),
        DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[1])};
    const WorkExtent elements = WorkExtent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const Heat2DKernel kernel = Heat2DKernel{};

    for (int step = 0; step < options.steps; ++step)
    {
        alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(deviceCurrent),
            alpaka::getPtrNative(deviceNextIntensity), alpaka::getPtrNative(deviceNextVelocity),
            static_cast<Idx>(options.width), static_cast<Idx>(options.height), options.heatIncrement,
            options.maxIntensity, options.timeStep);
        std::swap(deviceCurrent, deviceNextIntensity);
    }
    alpaka::wait(queue);

    result->intensity.resize(CellCount(options));
    result->velocity.resize(CellCount(options));
    HostView hostIntensity = alpaka::createView(hostDevice, result->intensity.data(), memExtent);
    HostView hostVelocity = alpaka::createView(hostDevice, result->velocity.data(), memExtent);
    alpaka::memcpy(queue, hostIntensity, deviceCurrent, memExtent);
    alpaka::memcpy(queue, hostVelocity, deviceNextVelocity, memExtent);
    alpaka::wait(queue);
}

AlpakaPlaneValue MaxHeat2DDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual)
{
    if (expected.size() != actual.size())
    {
        throw std::invalid_argument("CA_HEAT_2D comparison sizes differ");
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
