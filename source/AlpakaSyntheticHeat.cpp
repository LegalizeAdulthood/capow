#include "AlpakaSyntheticHeat.hpp"

#include "AlpakaUtilities.hpp"

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

const capow::AlpakaPlaneValue defaultDiffusion = capow::AlpakaPlaneValue(0.16F);

ALPAKA_FN_HOST_ACC capow::AlpakaPlaneValue ClampUnit(capow::AlpakaPlaneValue value)
{
    if (value < capow::AlpakaPlaneValue(0))
    {
        return capow::AlpakaPlaneValue(0);
    }
    if (value > capow::AlpakaPlaneValue(1))
    {
        return capow::AlpakaPlaneValue(1);
    }
    return value;
}

ALPAKA_FN_HOST_ACC capow::AlpakaPlaneValue Diffuse(capow::AlpakaPlaneValue center, capow::AlpakaPlaneValue north,
    capow::AlpakaPlaneValue south, capow::AlpakaPlaneValue east, capow::AlpakaPlaneValue west,
    capow::AlpakaPlaneValue diffusion)
{
    return ClampUnit(center + diffusion * (north + south + east + west - capow::AlpakaPlaneValue(4) * center));
}

struct SyntheticHeatKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, const capow::AlpakaPlaneValue *source,
        capow::AlpakaPlaneValue *target, Idx width, Idx height, capow::AlpakaPlaneValue diffusion) const
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
        target[indexes.center] = Diffuse(source[indexes.center], source[indexes.north], source[indexes.south],
            source[indexes.east], source[indexes.west], diffusion);
    }
};

void ValidateOptions(const capow::SyntheticHeat2DOptions &options)
{
    if (options.width <= 0 || options.height <= 0)
    {
        throw std::invalid_argument("synthetic heat dimensions must be positive");
    }
    if (options.steps < 0)
    {
        throw std::invalid_argument("synthetic heat steps must not be negative");
    }
}

std::size_t CellCount(const capow::SyntheticHeat2DOptions &options)
{
    return static_cast<std::size_t>(options.width) * static_cast<std::size_t>(options.height);
}

void ValidateFieldSize(const capow::SyntheticHeat2DOptions &options, const std::vector<capow::AlpakaPlaneValue> &field)
{
    if (field.size() != CellCount(options))
    {
        throw std::invalid_argument("synthetic heat field size does not match dimensions");
    }
}

capow::AlpakaPlaneValue DeterministicValue(std::uint32_t index)
{
    std::uint32_t value = index * 1664525U + 1013904223U + 0x1234U;
    value ^= value >> 16U;
    return static_cast<capow::AlpakaPlaneValue>(value & 0x00ffffffU) /
        static_cast<capow::AlpakaPlaneValue>(0x00ffffffU);
}

void SyntheticHeat2DStepHost(const capow::SyntheticHeat2DOptions &options,
    const std::vector<capow::AlpakaPlaneValue> &source, std::vector<capow::AlpakaPlaneValue> *target)
{
    const std::uint32_t width = static_cast<std::uint32_t>(options.width);
    const std::uint32_t height = static_cast<std::uint32_t>(options.height);
    for (std::uint32_t y = 0U; y < height; ++y)
    {
        for (std::uint32_t x = 0U; x < width; ++x)
        {
            const capow::alpaka_util::FiveNeighborIndexes indexes =
                capow::alpaka_util::five_neighbor_indexes(x, y, width, height);
            (*target)[indexes.center] = Diffuse(source[indexes.center], source[indexes.north], source[indexes.south],
                source[indexes.east], source[indexes.west], options.diffusion);
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

SyntheticHeat2DOptions::SyntheticHeat2DOptions() :
    width(0),
    height(0),
    steps(0),
    diffusion(defaultDiffusion)
{
}

void MakeSyntheticHeat2DInitial(const SyntheticHeat2DOptions &options, std::vector<AlpakaPlaneValue> *field)
{
    ValidateOptions(options);
    field->resize(CellCount(options));
    for (std::size_t index = 0; index < field->size(); ++index)
    {
        (*field)[index] = DeterministicValue(static_cast<std::uint32_t>(index));
    }
}

void RunSyntheticHeat2DHost(const SyntheticHeat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial,
    std::vector<AlpakaPlaneValue> *result)
{
    ValidateOptions(options);
    ValidateFieldSize(options, initial);

    std::vector<AlpakaPlaneValue> current = initial;
    std::vector<AlpakaPlaneValue> next(CellCount(options));
    for (int step = 0; step < options.steps; ++step)
    {
        SyntheticHeat2DStepHost(options, current, &next);
        std::swap(current, next);
    }
    *result = current;
}

void RunSyntheticHeat2DGpu(const SyntheticHeat2DOptions &options, const std::vector<AlpakaPlaneValue> &initial,
    std::vector<AlpakaPlaneValue> *result)
{
    ValidateOptions(options);
    ValidateFieldSize(options, initial);

    const AccPlatform accPlatform = AccPlatform{};
    const HostPlatform hostPlatform = HostPlatform{};
    const AccDevice accDevice = alpaka::getDevByIdx(accPlatform, 0U);
    const HostDevice hostDevice = alpaka::getDevByIdx(hostPlatform, 0U);
    Queue queue(accDevice);
    const MemExtent memExtent = MemExtent{static_cast<Idx>(CellCount(options))};

    DeviceBuffer deviceCurrent = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    DeviceBuffer deviceNext = alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, memExtent);
    std::vector<AlpakaPlaneValue> hostInitialData = initial;
    HostView hostInitial = alpaka::createView(hostDevice, hostInitialData.data(), memExtent);
    alpaka::memcpy(queue, deviceCurrent, hostInitial, memExtent);
    alpaka::wait(queue);

    const WorkExtent threads = WorkExtent{16U, 16U};
    const WorkExtent blocks = WorkExtent{DivideRoundUp(static_cast<std::uint32_t>(options.height), threads[0]),
        DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[1])};
    const WorkExtent elements = WorkExtent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const SyntheticHeatKernel kernel = SyntheticHeatKernel{};

    for (int step = 0; step < options.steps; ++step)
    {
        alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(deviceCurrent),
            alpaka::getPtrNative(deviceNext), static_cast<Idx>(options.width), static_cast<Idx>(options.height),
            options.diffusion);
        std::swap(deviceCurrent, deviceNext);
    }
    alpaka::wait(queue);

    result->resize(CellCount(options));
    HostView hostResult = alpaka::createView(hostDevice, result->data(), memExtent);
    alpaka::memcpy(queue, hostResult, deviceCurrent, memExtent);
    alpaka::wait(queue);
}

AlpakaPlaneValue MaxAbsDifference(
    const std::vector<AlpakaPlaneValue> &expected, const std::vector<AlpakaPlaneValue> &actual)
{
    if (expected.size() != actual.size())
    {
        throw std::invalid_argument("synthetic heat comparison sizes differ");
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
