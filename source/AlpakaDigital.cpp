#include "AlpakaDigital.hpp"

#include "CapowRules.hpp"

#include <alpaka/alpaka.hpp>

#include <algorithm>
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
using HostDigitalView = alpaka::ViewPlainPtr<HostDevice, capow::AlpakaDigitalValue, MemDim, Idx>;
using ConstHostDigitalView = alpaka::ViewPlainPtr<HostDevice, const capow::AlpakaDigitalValue, MemDim, Idx>;
using DigitalBuffer = alpaka::Buf<AccPlatform, capow::AlpakaDigitalValue, MemDim, Idx>;

struct StandardDigitalKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, const capow::AlpakaDigitalValue *source,
        const capow::AlpakaDigitalValue *lookup, capow::AlpakaDigitalValue *target, Idx width, Idx radius,
        Idx stateBits) const
    {
        const Idx x = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0];
        if (x >= width)
        {
            return;
        }

        target[x] = capow::ComputeStandardDigitalCellWrap(source, lookup, x, width, radius, stateBits);
    }
};

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor)
{
    return (value + divisor - 1U) / divisor;
}

bool IsPowerOfTwo(int value)
{
    return value > 0 && (value & (value - 1)) == 0;
}

int LookupCountFor(const capow::StandardDigitalOptions &options)
{
    const int nabeSize = 1 + 2 * options.radius;
    int result = 1;
    for (int i = 0; i < nabeSize; ++i)
    {
        result *= options.stateCount;
    }
    return result;
}

std::size_t CellCount(const capow::StandardDigitalOptions &options)
{
    return static_cast<std::size_t>(options.width);
}

void ValidateOptions(const capow::StandardDigitalOptions &options)
{
    if (options.width <= 2 * options.radius)
    {
        throw std::invalid_argument("CA_STANDARD width must exceed its diameter");
    }
    if (options.steps < 0)
    {
        throw std::invalid_argument("CA_STANDARD steps must not be negative");
    }
    if (!IsPowerOfTwo(options.stateCount) || options.stateCount > 256)
    {
        throw std::invalid_argument("CA_STANDARD state count must be a byte-sized power of two");
    }
    if (options.stateBits <= 0 || options.stateBits > 8 || (1 << options.stateBits) != options.stateCount)
    {
        throw std::invalid_argument("CA_STANDARD state bits must match the state count");
    }
    if (options.radius <= 0)
    {
        throw std::invalid_argument("CA_STANDARD radius must be positive");
    }
    if (options.lookupCount <= 0 || options.lookupCount != LookupCountFor(options))
    {
        throw std::invalid_argument("CA_STANDARD lookup count does not match radius and states");
    }
}

void ValidateRowSize(const capow::StandardDigitalOptions &options, const std::vector<capow::AlpakaDigitalValue> &row)
{
    if (row.size() != CellCount(options))
    {
        throw std::invalid_argument("CA_STANDARD row size does not match width");
    }
}

void ValidateLookupSize(
    const capow::StandardDigitalOptions &options, const std::vector<capow::AlpakaDigitalValue> &lookup)
{
    if (lookup.size() != static_cast<std::size_t>(options.lookupCount))
    {
        throw std::invalid_argument("CA_STANDARD lookup size does not match options");
    }
}

std::uint32_t UnitValue(std::uint32_t index)
{
    std::uint32_t value = index * 1103515245U + 12345U + 0x51A7U;
    value ^= value >> 16U;
    value *= 2246822519U;
    value ^= value >> 13U;
    return value;
}

void StandardDigitalStepHost(const capow::StandardDigitalOptions &options,
    const std::vector<capow::AlpakaDigitalValue> &source, const std::vector<capow::AlpakaDigitalValue> &lookup,
    std::vector<capow::AlpakaDigitalValue> *target)
{
    for (std::uint32_t x = 0U; x < static_cast<std::uint32_t>(options.width); ++x)
    {
        (*target)[x] = capow::ComputeStandardDigitalCellWrap(source.data(), lookup.data(), x,
            static_cast<std::uint32_t>(options.width), static_cast<std::uint32_t>(options.radius),
            static_cast<std::uint32_t>(options.stateBits));
    }
}

} // namespace

namespace capow
{

StandardDigitalOptions::StandardDigitalOptions() :
    width(0),
    steps(0),
    stateCount(16),
    radius(1),
    stateBits(4),
    lookupCount(4096)
{
}

void MakeStandardDigitalInitial(const StandardDigitalOptions &options, std::vector<AlpakaDigitalValue> *source)
{
    ValidateOptions(options);
    source->resize(CellCount(options));
    for (std::size_t index = 0; index < source->size(); ++index)
    {
        (*source)[index] = static_cast<AlpakaDigitalValue>(
            UnitValue(static_cast<std::uint32_t>(index)) % static_cast<std::uint32_t>(options.stateCount));
    }
}

void MakeStandardDigitalLookup(const StandardDigitalOptions &options, std::vector<AlpakaDigitalValue> *lookup)
{
    ValidateOptions(options);
    lookup->resize(static_cast<std::size_t>(options.lookupCount));
    for (std::size_t index = 0; index < lookup->size(); ++index)
    {
        const std::uint32_t value = UnitValue(static_cast<std::uint32_t>(index + lookup->size()));
        (*lookup)[index] = static_cast<AlpakaDigitalValue>(value % static_cast<std::uint32_t>(options.stateCount));
    }
}

void RunStandardDigitalHost(const StandardDigitalOptions &options, const std::vector<AlpakaDigitalValue> &initialSource,
    const std::vector<AlpakaDigitalValue> &lookup, std::vector<AlpakaDigitalValue> *result)
{
    ValidateOptions(options);
    ValidateRowSize(options, initialSource);
    ValidateLookupSize(options, lookup);

    std::vector<AlpakaDigitalValue> current = initialSource;
    std::vector<AlpakaDigitalValue> next(CellCount(options), AlpakaDigitalValue(0));
    for (int step = 0; step < options.steps; ++step)
    {
        StandardDigitalStepHost(options, current, lookup, &next);
        std::swap(current, next);
    }
    *result = current;
}

void RunStandardDigitalGpu(const StandardDigitalOptions &options, const std::vector<AlpakaDigitalValue> &initialSource,
    const std::vector<AlpakaDigitalValue> &lookup, std::vector<AlpakaDigitalValue> *result)
{
    ValidateOptions(options);
    ValidateRowSize(options, initialSource);
    ValidateLookupSize(options, lookup);

    if (options.steps == 0)
    {
        *result = initialSource;
        return;
    }

    const AccPlatform accPlatform = AccPlatform{};
    const HostPlatform hostPlatform = HostPlatform{};
    const AccDevice accDevice = alpaka::getDevByIdx(accPlatform, 0U);
    const HostDevice hostDevice = alpaka::getDevByIdx(hostPlatform, 0U);
    Queue queue(accDevice);
    const Extent rowExtent = Extent{static_cast<Idx>(CellCount(options))};
    const Extent lookupExtent = Extent{static_cast<Idx>(lookup.size())};

    DigitalBuffer deviceCurrent = alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, rowExtent);
    DigitalBuffer deviceNext = alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, rowExtent);
    DigitalBuffer deviceLookup = alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, lookupExtent);
    ConstHostDigitalView hostInitial = alpaka::createView(hostDevice, initialSource.data(), rowExtent);
    ConstHostDigitalView hostLookup = alpaka::createView(hostDevice, lookup.data(), lookupExtent);
    alpaka::memcpy(queue, deviceCurrent, hostInitial, rowExtent);
    alpaka::memcpy(queue, deviceLookup, hostLookup, lookupExtent);
    alpaka::wait(queue);

    const Extent threads = Extent{128U};
    const Extent blocks = Extent{DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[0])};
    const Extent elements = Extent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const StandardDigitalKernel kernel = StandardDigitalKernel{};
    for (int step = 0; step < options.steps; ++step)
    {
        alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(deviceCurrent),
            alpaka::getPtrNative(deviceLookup), alpaka::getPtrNative(deviceNext), static_cast<Idx>(options.width),
            static_cast<Idx>(options.radius), static_cast<Idx>(options.stateBits));
        std::swap(deviceCurrent, deviceNext);
    }
    alpaka::wait(queue);

    result->resize(CellCount(options));
    HostDigitalView hostResult = alpaka::createView(hostDevice, result->data(), rowExtent);
    alpaka::memcpy(queue, hostResult, deviceCurrent, rowExtent);
    alpaka::wait(queue);
}

int CountStandardDigitalDifferences(
    const std::vector<AlpakaDigitalValue> &expected, const std::vector<AlpakaDigitalValue> &actual)
{
    if (expected.size() != actual.size())
    {
        throw std::invalid_argument("CA_STANDARD comparison sizes differ");
    }

    int differenceCount = 0;
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        if (expected[index] != actual[index])
        {
            ++differenceCount;
        }
    }
    return differenceCount;
}

void NormalizeStandardDigitalRow(const StandardDigitalOptions &options, const std::vector<AlpakaDigitalValue> &row,
    std::vector<AlpakaPlaneValue> *normalized)
{
    ValidateOptions(options);
    ValidateRowSize(options, row);
    normalized->resize(row.size());
    const AlpakaPlaneValue divisor = static_cast<AlpakaPlaneValue>(options.stateCount - 1);
    for (std::size_t index = 0; index < row.size(); ++index)
    {
        (*normalized)[index] = static_cast<AlpakaPlaneValue>(row[index]) / divisor;
    }
}

} // namespace capow
