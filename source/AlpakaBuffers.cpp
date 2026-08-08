#include "AlpakaBuffers.hpp"

#include <alpaka/alpaka.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace
{

using Dim = alpaka::DimInt<1U>;
using Idx = std::uint32_t;
using Acc = alpaka::AccGpuCudaRt<Dim, Idx>;
using AccPlatform = alpaka::Platform<Acc>;
using HostPlatform = alpaka::PlatformCpu;
using AccDevice = alpaka::Dev<AccPlatform>;
using HostDevice = alpaka::Dev<HostPlatform>;
using Queue = alpaka::Queue<Acc, alpaka::Blocking>;
using Extent = alpaka::Vec<Dim, Idx>;
using HostView = alpaka::ViewPlainPtr<HostDevice, capow::AlpakaPlaneValue, Dim, Idx>;
using HostDigitalView = alpaka::ViewPlainPtr<HostDevice, capow::AlpakaDigitalValue, Dim, Idx>;
using PlaneBuffer = alpaka::Buf<AccPlatform, capow::AlpakaPlaneValue, Dim, Idx>;
using DigitalBuffer = alpaka::Buf<AccPlatform, capow::AlpakaDigitalValue, Dim, Idx>;

constexpr int rowSlotCount = 3;
constexpr int initialSourceSlot = 0;
constexpr int initialTargetSlot = 1;
constexpr int initialPastSlot = 2;

void ValidateDimensions(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        throw std::invalid_argument("Alpaka 2D plane dimensions must be positive");
    }
}

void ValidatePlane(const capow::AlpakaPlaneValue *plane, int valueStride)
{
    if (plane == nullptr)
    {
        throw std::invalid_argument("Alpaka 2D plane pointer must not be null");
    }
    if (valueStride <= 0)
    {
        throw std::invalid_argument("Alpaka 2D plane value stride must be positive");
    }
}

std::size_t CellCountForDimensions(int width, int height)
{
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

void ValidateRowWidth(int width)
{
    if (width <= 0)
    {
        throw std::invalid_argument("Alpaka 1D row width must be positive");
    }
}

void ValidateRow(const capow::AlpakaPlaneValue *row, int valueStride)
{
    if (row == nullptr)
    {
        throw std::invalid_argument("Alpaka 1D row pointer must not be null");
    }
    if (valueStride <= 0)
    {
        throw std::invalid_argument("Alpaka 1D row value stride must be positive");
    }
}

void ValidateDigitalRow(const capow::AlpakaDigitalValue *row)
{
    if (row == nullptr)
    {
        throw std::invalid_argument("Alpaka digital row pointer must not be null");
    }
}

void ValidateLookupCount(int lookupCount)
{
    if (lookupCount <= 0)
    {
        throw std::invalid_argument("Alpaka digital lookup count must be positive");
    }
}

void ValidateLookup(const capow::AlpakaDigitalValue *lookup)
{
    if (lookup == nullptr)
    {
        throw std::invalid_argument("Alpaka digital lookup pointer must not be null");
    }
}

std::size_t CellCountForWidth(int width)
{
    return static_cast<std::size_t>(width);
}

int NextSlot(int slot)
{
    return (slot + 1) % rowSlotCount;
}

} // namespace

namespace capow
{

class AlpakaPlaneMirror2D::Impl
{
public:
    Impl();

    bool IsInitialized() const;
    bool IsDirty() const;
    int GetWidth() const;
    int GetHeight() const;
    std::size_t GetCellCount() const;

    void Resize(int nextWidth, int nextHeight);
    void MarkDirty();
    void CopySourceAndPastToDevice(
        const AlpakaPlaneValue *sourcePlane, const AlpakaPlaneValue *pastPlane, int valueStride);
    void DebugCopySourceToTarget();
    void DebugCopyPastToTarget();
    void CopyTargetToHost(AlpakaPlaneValue *targetPlane, int valueStride);

private:
    void RequireInitialized() const;
    void PackPlane(std::vector<AlpakaPlaneValue> *hostPlane, const AlpakaPlaneValue *plane, int valueStride) const;
    void UnpackPlane(AlpakaPlaneValue *plane, int valueStride) const;
    void CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostPlane, PlaneBuffer *devicePlane);
    void CopyDeviceToTarget(const PlaneBuffer &devicePlane);

    AccDevice accDevice;
    HostDevice hostDevice;
    Queue queue;
    int width;
    int height;
    std::size_t cellCount;
    bool initialized;
    bool dirty;
    std::vector<AlpakaPlaneValue> hostSource;
    std::vector<AlpakaPlaneValue> hostPast;
    std::vector<AlpakaPlaneValue> hostTarget;
    std::optional<PlaneBuffer> deviceSource;
    std::optional<PlaneBuffer> devicePast;
    std::optional<PlaneBuffer> deviceTarget;
};

AlpakaPlaneMirror2D::Impl::Impl() :
    accDevice(alpaka::getDevByIdx(AccPlatform{}, 0U)),
    hostDevice(alpaka::getDevByIdx(HostPlatform{}, 0U)),
    queue(accDevice),
    width(0),
    height(0),
    cellCount(0U),
    initialized(false),
    dirty(false)
{
}

bool AlpakaPlaneMirror2D::Impl::IsInitialized() const
{
    return initialized;
}

bool AlpakaPlaneMirror2D::Impl::IsDirty() const
{
    return dirty;
}

int AlpakaPlaneMirror2D::Impl::GetWidth() const
{
    return width;
}

int AlpakaPlaneMirror2D::Impl::GetHeight() const
{
    return height;
}

std::size_t AlpakaPlaneMirror2D::Impl::GetCellCount() const
{
    return cellCount;
}

void AlpakaPlaneMirror2D::Impl::Resize(int nextWidth, int nextHeight)
{
    ValidateDimensions(nextWidth, nextHeight);

    const std::size_t nextCellCount = CellCountForDimensions(nextWidth, nextHeight);
    if (initialized && width == nextWidth && height == nextHeight)
    {
        dirty = true;
        return;
    }

    const Extent extent = Extent{static_cast<Idx>(nextCellCount)};
    deviceSource.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent));
    devicePast.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent));
    deviceTarget.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent));

    width = nextWidth;
    height = nextHeight;
    cellCount = nextCellCount;
    initialized = true;
    dirty = true;
    hostSource.assign(cellCount, AlpakaPlaneValue(0));
    hostPast.assign(cellCount, AlpakaPlaneValue(0));
    hostTarget.assign(cellCount, AlpakaPlaneValue(0));
}

void AlpakaPlaneMirror2D::Impl::MarkDirty()
{
    dirty = true;
}

void AlpakaPlaneMirror2D::Impl::CopySourceAndPastToDevice(
    const AlpakaPlaneValue *sourcePlane, const AlpakaPlaneValue *pastPlane, int valueStride)
{
    RequireInitialized();
    PackPlane(&hostSource, sourcePlane, valueStride);
    PackPlane(&hostPast, pastPlane, valueStride);
    CopyHostToDevice(&hostSource, &*deviceSource);
    CopyHostToDevice(&hostPast, &*devicePast);
    dirty = false;
}

void AlpakaPlaneMirror2D::Impl::DebugCopySourceToTarget()
{
    RequireInitialized();
    CopyDeviceToTarget(*deviceSource);
}

void AlpakaPlaneMirror2D::Impl::DebugCopyPastToTarget()
{
    RequireInitialized();
    CopyDeviceToTarget(*devicePast);
}

void AlpakaPlaneMirror2D::Impl::CopyTargetToHost(AlpakaPlaneValue *targetPlane, int valueStride)
{
    RequireInitialized();
    ValidatePlane(targetPlane, valueStride);

    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    HostView hostTargetView = alpaka::createView(hostDevice, hostTarget.data(), extent);
    alpaka::memcpy(queue, hostTargetView, *deviceTarget, extent);
    alpaka::wait(queue);
    UnpackPlane(targetPlane, valueStride);
}

void AlpakaPlaneMirror2D::Impl::RequireInitialized() const
{
    if (!initialized)
    {
        throw std::logic_error("Alpaka 2D plane mirror is not initialized");
    }
}

void AlpakaPlaneMirror2D::Impl::PackPlane(
    std::vector<AlpakaPlaneValue> *hostPlane, const AlpakaPlaneValue *plane, int valueStride) const
{
    ValidatePlane(plane, valueStride);
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        (*hostPlane)[index] = plane[index * static_cast<std::size_t>(valueStride)];
    }
}

void AlpakaPlaneMirror2D::Impl::UnpackPlane(AlpakaPlaneValue *plane, int valueStride) const
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        plane[index * static_cast<std::size_t>(valueStride)] = hostTarget[index];
    }
}

void AlpakaPlaneMirror2D::Impl::CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostPlane, PlaneBuffer *devicePlane)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    HostView hostView = alpaka::createView(hostDevice, hostPlane->data(), extent);
    alpaka::memcpy(queue, *devicePlane, hostView, extent);
    alpaka::wait(queue);
}

void AlpakaPlaneMirror2D::Impl::CopyDeviceToTarget(const PlaneBuffer &devicePlane)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    alpaka::memcpy(queue, *deviceTarget, devicePlane, extent);
    alpaka::wait(queue);
}

AlpakaPlaneMirror2D::AlpakaPlaneMirror2D() :
    impl(std::make_unique<Impl>())
{
}

AlpakaPlaneMirror2D::~AlpakaPlaneMirror2D() = default;

bool AlpakaPlaneMirror2D::IsInitialized() const
{
    return impl->IsInitialized();
}

bool AlpakaPlaneMirror2D::IsDirty() const
{
    return impl->IsDirty();
}

int AlpakaPlaneMirror2D::GetWidth() const
{
    return impl->GetWidth();
}

int AlpakaPlaneMirror2D::GetHeight() const
{
    return impl->GetHeight();
}

std::size_t AlpakaPlaneMirror2D::GetCellCount() const
{
    return impl->GetCellCount();
}

void AlpakaPlaneMirror2D::Resize(int width, int height)
{
    impl->Resize(width, height);
}

void AlpakaPlaneMirror2D::MarkDirty()
{
    impl->MarkDirty();
}

void AlpakaPlaneMirror2D::CopySourceAndPastToDevice(
    const AlpakaPlaneValue *sourcePlane, const AlpakaPlaneValue *pastPlane, int valueStride)
{
    impl->CopySourceAndPastToDevice(sourcePlane, pastPlane, valueStride);
}

void AlpakaPlaneMirror2D::DebugCopySourceToTarget()
{
    impl->DebugCopySourceToTarget();
}

void AlpakaPlaneMirror2D::DebugCopyPastToTarget()
{
    impl->DebugCopyPastToTarget();
}

void AlpakaPlaneMirror2D::CopyTargetToHost(AlpakaPlaneValue *targetPlane, int valueStride)
{
    impl->CopyTargetToHost(targetPlane, valueStride);
}

class AlpakaContinuousRowMirror1D::Impl
{
public:
    Impl();

    bool IsInitialized() const;
    bool IsDirty() const;
    int GetWidth() const;
    std::size_t GetCellCount() const;
    int GetSourceSlot() const;
    int GetTargetSlot() const;
    int GetPastSlot() const;

    void Resize(int nextWidth);
    void MarkDirty();
    void CopySourceAndPastToDevice(const AlpakaPlaneValue *sourceIntensity, const AlpakaPlaneValue *sourceVelocity,
        const AlpakaPlaneValue *pastIntensity, const AlpakaPlaneValue *pastVelocity, int valueStride);
    void DebugCopySourceToTarget();
    void DebugCopyPastToTarget();
    void CopyTargetToDisplayRow();
    void RotateRows();
    void CopyTargetToHost(AlpakaPlaneValue *targetIntensity, AlpakaPlaneValue *targetVelocity, int valueStride);
    void CopyDisplayRowToHost(AlpakaPlaneValue *displayIntensity, AlpakaPlaneValue *displayVelocity, int valueStride);

private:
    void RequireInitialized() const;
    void ResetSlots();
    void PackRow(std::vector<AlpakaPlaneValue> *hostRow, const AlpakaPlaneValue *row, int valueStride) const;
    void UnpackRow(const std::vector<AlpakaPlaneValue> &hostRow, AlpakaPlaneValue *row, int valueStride) const;
    void CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostRow, PlaneBuffer *deviceRow);
    void CopyDeviceToHost(const PlaneBuffer &deviceRow, std::vector<AlpakaPlaneValue> *hostRow);
    void CopyDeviceRowsToTarget(int sourceRowSlot);
    void CopyDeviceRowToDisplay();

    AccDevice accDevice;
    HostDevice hostDevice;
    Queue queue;
    int width;
    std::size_t cellCount;
    bool initialized;
    bool dirty;
    int sourceSlot;
    int targetSlot;
    int pastSlot;
    std::array<std::vector<AlpakaPlaneValue>, rowSlotCount> hostIntensity;
    std::array<std::vector<AlpakaPlaneValue>, rowSlotCount> hostVelocity;
    std::vector<AlpakaPlaneValue> hostDisplayIntensity;
    std::vector<AlpakaPlaneValue> hostDisplayVelocity;
    std::array<std::optional<PlaneBuffer>, rowSlotCount> deviceIntensity;
    std::array<std::optional<PlaneBuffer>, rowSlotCount> deviceVelocity;
    std::optional<PlaneBuffer> deviceDisplayIntensity;
    std::optional<PlaneBuffer> deviceDisplayVelocity;
};

AlpakaContinuousRowMirror1D::Impl::Impl() :
    accDevice(alpaka::getDevByIdx(AccPlatform{}, 0U)),
    hostDevice(alpaka::getDevByIdx(HostPlatform{}, 0U)),
    queue(accDevice),
    width(0),
    cellCount(0U),
    initialized(false),
    dirty(false),
    sourceSlot(initialSourceSlot),
    targetSlot(initialTargetSlot),
    pastSlot(initialPastSlot)
{
}

bool AlpakaContinuousRowMirror1D::Impl::IsInitialized() const
{
    return initialized;
}

bool AlpakaContinuousRowMirror1D::Impl::IsDirty() const
{
    return dirty;
}

int AlpakaContinuousRowMirror1D::Impl::GetWidth() const
{
    return width;
}

std::size_t AlpakaContinuousRowMirror1D::Impl::GetCellCount() const
{
    return cellCount;
}

int AlpakaContinuousRowMirror1D::Impl::GetSourceSlot() const
{
    return sourceSlot;
}

int AlpakaContinuousRowMirror1D::Impl::GetTargetSlot() const
{
    return targetSlot;
}

int AlpakaContinuousRowMirror1D::Impl::GetPastSlot() const
{
    return pastSlot;
}

void AlpakaContinuousRowMirror1D::Impl::Resize(int nextWidth)
{
    ValidateRowWidth(nextWidth);

    const std::size_t nextCellCount = CellCountForWidth(nextWidth);
    if (initialized && width == nextWidth)
    {
        dirty = true;
        return;
    }

    const Extent extent = Extent{static_cast<Idx>(nextCellCount)};
    for (int slot = 0; slot < rowSlotCount; ++slot)
    {
        deviceIntensity[slot].emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent));
        deviceVelocity[slot].emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent));
        hostIntensity[slot].assign(nextCellCount, AlpakaPlaneValue(0));
        hostVelocity[slot].assign(nextCellCount, AlpakaPlaneValue(0));
    }
    deviceDisplayIntensity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent));
    deviceDisplayVelocity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, extent));

    width = nextWidth;
    cellCount = nextCellCount;
    initialized = true;
    dirty = true;
    ResetSlots();
    hostDisplayIntensity.assign(cellCount, AlpakaPlaneValue(0));
    hostDisplayVelocity.assign(cellCount, AlpakaPlaneValue(0));
}

void AlpakaContinuousRowMirror1D::Impl::MarkDirty()
{
    dirty = true;
}

void AlpakaContinuousRowMirror1D::Impl::CopySourceAndPastToDevice(const AlpakaPlaneValue *sourceIntensity,
    const AlpakaPlaneValue *sourceVelocity, const AlpakaPlaneValue *pastIntensity, const AlpakaPlaneValue *pastVelocity,
    int valueStride)
{
    RequireInitialized();
    PackRow(&hostIntensity[sourceSlot], sourceIntensity, valueStride);
    PackRow(&hostVelocity[sourceSlot], sourceVelocity, valueStride);
    PackRow(&hostIntensity[pastSlot], pastIntensity, valueStride);
    PackRow(&hostVelocity[pastSlot], pastVelocity, valueStride);
    CopyHostToDevice(&hostIntensity[sourceSlot], &*deviceIntensity[sourceSlot]);
    CopyHostToDevice(&hostVelocity[sourceSlot], &*deviceVelocity[sourceSlot]);
    CopyHostToDevice(&hostIntensity[pastSlot], &*deviceIntensity[pastSlot]);
    CopyHostToDevice(&hostVelocity[pastSlot], &*deviceVelocity[pastSlot]);
    dirty = false;
}

void AlpakaContinuousRowMirror1D::Impl::DebugCopySourceToTarget()
{
    RequireInitialized();
    CopyDeviceRowsToTarget(sourceSlot);
}

void AlpakaContinuousRowMirror1D::Impl::DebugCopyPastToTarget()
{
    RequireInitialized();
    CopyDeviceRowsToTarget(pastSlot);
}

void AlpakaContinuousRowMirror1D::Impl::CopyTargetToDisplayRow()
{
    RequireInitialized();
    CopyDeviceRowToDisplay();
}

void AlpakaContinuousRowMirror1D::Impl::RotateRows()
{
    RequireInitialized();
    sourceSlot = NextSlot(sourceSlot);
    targetSlot = NextSlot(targetSlot);
    pastSlot = NextSlot(pastSlot);
}

void AlpakaContinuousRowMirror1D::Impl::CopyTargetToHost(
    AlpakaPlaneValue *targetIntensity, AlpakaPlaneValue *targetVelocity, int valueStride)
{
    RequireInitialized();
    ValidateRow(targetIntensity, valueStride);
    ValidateRow(targetVelocity, valueStride);

    CopyDeviceToHost(*deviceIntensity[targetSlot], &hostIntensity[targetSlot]);
    CopyDeviceToHost(*deviceVelocity[targetSlot], &hostVelocity[targetSlot]);
    UnpackRow(hostIntensity[targetSlot], targetIntensity, valueStride);
    UnpackRow(hostVelocity[targetSlot], targetVelocity, valueStride);
}

void AlpakaContinuousRowMirror1D::Impl::CopyDisplayRowToHost(
    AlpakaPlaneValue *displayIntensity, AlpakaPlaneValue *displayVelocity, int valueStride)
{
    RequireInitialized();
    ValidateRow(displayIntensity, valueStride);
    ValidateRow(displayVelocity, valueStride);

    CopyDeviceToHost(*deviceDisplayIntensity, &hostDisplayIntensity);
    CopyDeviceToHost(*deviceDisplayVelocity, &hostDisplayVelocity);
    UnpackRow(hostDisplayIntensity, displayIntensity, valueStride);
    UnpackRow(hostDisplayVelocity, displayVelocity, valueStride);
}

void AlpakaContinuousRowMirror1D::Impl::RequireInitialized() const
{
    if (!initialized)
    {
        throw std::logic_error("Alpaka 1D row mirror is not initialized");
    }
}

void AlpakaContinuousRowMirror1D::Impl::ResetSlots()
{
    sourceSlot = initialSourceSlot;
    targetSlot = initialTargetSlot;
    pastSlot = initialPastSlot;
}

void AlpakaContinuousRowMirror1D::Impl::PackRow(
    std::vector<AlpakaPlaneValue> *hostRow, const AlpakaPlaneValue *row, int valueStride) const
{
    ValidateRow(row, valueStride);
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        (*hostRow)[index] = row[index * static_cast<std::size_t>(valueStride)];
    }
}

void AlpakaContinuousRowMirror1D::Impl::UnpackRow(
    const std::vector<AlpakaPlaneValue> &hostRow, AlpakaPlaneValue *row, int valueStride) const
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        row[index * static_cast<std::size_t>(valueStride)] = hostRow[index];
    }
}

void AlpakaContinuousRowMirror1D::Impl::CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostRow, PlaneBuffer *deviceRow)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    HostView hostView = alpaka::createView(hostDevice, hostRow->data(), extent);
    alpaka::memcpy(queue, *deviceRow, hostView, extent);
    alpaka::wait(queue);
}

void AlpakaContinuousRowMirror1D::Impl::CopyDeviceToHost(
    const PlaneBuffer &deviceRow, std::vector<AlpakaPlaneValue> *hostRow)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    HostView hostView = alpaka::createView(hostDevice, hostRow->data(), extent);
    alpaka::memcpy(queue, hostView, deviceRow, extent);
    alpaka::wait(queue);
}

void AlpakaContinuousRowMirror1D::Impl::CopyDeviceRowsToTarget(int sourceRowSlot)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    alpaka::memcpy(queue, *deviceIntensity[targetSlot], *deviceIntensity[sourceRowSlot], extent);
    alpaka::memcpy(queue, *deviceVelocity[targetSlot], *deviceVelocity[sourceRowSlot], extent);
    alpaka::wait(queue);
}

void AlpakaContinuousRowMirror1D::Impl::CopyDeviceRowToDisplay()
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    alpaka::memcpy(queue, *deviceDisplayIntensity, *deviceIntensity[targetSlot], extent);
    alpaka::memcpy(queue, *deviceDisplayVelocity, *deviceVelocity[targetSlot], extent);
    alpaka::wait(queue);
}

AlpakaContinuousRowMirror1D::AlpakaContinuousRowMirror1D() :
    impl(std::make_unique<Impl>())
{
}

AlpakaContinuousRowMirror1D::~AlpakaContinuousRowMirror1D() = default;

bool AlpakaContinuousRowMirror1D::IsInitialized() const
{
    return impl->IsInitialized();
}

bool AlpakaContinuousRowMirror1D::IsDirty() const
{
    return impl->IsDirty();
}

int AlpakaContinuousRowMirror1D::GetWidth() const
{
    return impl->GetWidth();
}

std::size_t AlpakaContinuousRowMirror1D::GetCellCount() const
{
    return impl->GetCellCount();
}

int AlpakaContinuousRowMirror1D::GetSourceSlot() const
{
    return impl->GetSourceSlot();
}

int AlpakaContinuousRowMirror1D::GetTargetSlot() const
{
    return impl->GetTargetSlot();
}

int AlpakaContinuousRowMirror1D::GetPastSlot() const
{
    return impl->GetPastSlot();
}

void AlpakaContinuousRowMirror1D::Resize(int width)
{
    impl->Resize(width);
}

void AlpakaContinuousRowMirror1D::MarkDirty()
{
    impl->MarkDirty();
}

void AlpakaContinuousRowMirror1D::CopySourceAndPastToDevice(const AlpakaPlaneValue *sourceIntensity,
    const AlpakaPlaneValue *sourceVelocity, const AlpakaPlaneValue *pastIntensity, const AlpakaPlaneValue *pastVelocity,
    int valueStride)
{
    impl->CopySourceAndPastToDevice(sourceIntensity, sourceVelocity, pastIntensity, pastVelocity, valueStride);
}

void AlpakaContinuousRowMirror1D::DebugCopySourceToTarget()
{
    impl->DebugCopySourceToTarget();
}

void AlpakaContinuousRowMirror1D::DebugCopyPastToTarget()
{
    impl->DebugCopyPastToTarget();
}

void AlpakaContinuousRowMirror1D::CopyTargetToDisplayRow()
{
    impl->CopyTargetToDisplayRow();
}

void AlpakaContinuousRowMirror1D::RotateRows()
{
    impl->RotateRows();
}

void AlpakaContinuousRowMirror1D::CopyTargetToHost(
    AlpakaPlaneValue *targetIntensity, AlpakaPlaneValue *targetVelocity, int valueStride)
{
    impl->CopyTargetToHost(targetIntensity, targetVelocity, valueStride);
}

void AlpakaContinuousRowMirror1D::CopyDisplayRowToHost(
    AlpakaPlaneValue *displayIntensity, AlpakaPlaneValue *displayVelocity, int valueStride)
{
    impl->CopyDisplayRowToHost(displayIntensity, displayVelocity, valueStride);
}

class AlpakaDigitalRowMirror1D::Impl
{
public:
    Impl();

    bool IsInitialized() const;
    bool IsDirty() const;
    bool HasPastRow() const;
    int GetWidth() const;
    int GetLookupCount() const;
    std::size_t GetCellCount() const;
    int GetSourceSlot() const;
    int GetTargetSlot() const;
    int GetPastSlot() const;

    void Resize(int nextWidth, int nextLookupCount, bool nextHasPastRow);
    void MarkDirty();
    void CopyRowsAndLookupToDevice(const AlpakaDigitalValue *sourceRow, const AlpakaDigitalValue *targetRow,
        const AlpakaDigitalValue *pastRow, const AlpakaDigitalValue *lookup);
    void DebugCopySourceToTarget();
    void DebugCopyPastToTarget();
    void RotateRows();
    void CopySourceToHost(AlpakaDigitalValue *sourceRow);
    void CopyTargetToHost(AlpakaDigitalValue *targetRow);
    void CopyPastToHost(AlpakaDigitalValue *pastRow);
    void CopyLookupToHost(AlpakaDigitalValue *lookup);

private:
    void RequireInitialized() const;
    void RequirePastRow() const;
    void ResetSlots();
    void PackRow(std::vector<AlpakaDigitalValue> *hostRow, const AlpakaDigitalValue *row) const;
    void PackLookup(const AlpakaDigitalValue *lookup);
    void UnpackRow(const std::vector<AlpakaDigitalValue> &hostRow, AlpakaDigitalValue *row) const;
    void UnpackLookup(AlpakaDigitalValue *lookup) const;
    void CopyHostToDevice(
        std::vector<AlpakaDigitalValue> *hostBuffer, DigitalBuffer *deviceBuffer, std::size_t valueCount);
    void CopyDeviceToHost(
        DigitalBuffer *deviceBuffer, std::vector<AlpakaDigitalValue> *hostBuffer, std::size_t valueCount);
    void CopyDeviceRowToTarget(int sourceRowSlot);

    AccDevice accDevice;
    HostDevice hostDevice;
    Queue queue;
    int width;
    int lookupCount;
    std::size_t cellCount;
    std::size_t lookupCellCount;
    bool initialized;
    bool dirty;
    bool hasPastRow;
    int sourceSlot;
    int targetSlot;
    int pastSlot;
    std::array<std::vector<AlpakaDigitalValue>, rowSlotCount> hostRows;
    std::vector<AlpakaDigitalValue> hostLookup;
    std::array<std::optional<DigitalBuffer>, rowSlotCount> deviceRows;
    std::optional<DigitalBuffer> deviceLookup;
};

AlpakaDigitalRowMirror1D::Impl::Impl() :
    accDevice(alpaka::getDevByIdx(AccPlatform{}, 0U)),
    hostDevice(alpaka::getDevByIdx(HostPlatform{}, 0U)),
    queue(accDevice),
    width(0),
    lookupCount(0),
    cellCount(0U),
    lookupCellCount(0U),
    initialized(false),
    dirty(false),
    hasPastRow(false),
    sourceSlot(initialSourceSlot),
    targetSlot(initialTargetSlot),
    pastSlot(initialPastSlot)
{
}

bool AlpakaDigitalRowMirror1D::Impl::IsInitialized() const
{
    return initialized;
}

bool AlpakaDigitalRowMirror1D::Impl::IsDirty() const
{
    return dirty;
}

bool AlpakaDigitalRowMirror1D::Impl::HasPastRow() const
{
    return hasPastRow;
}

int AlpakaDigitalRowMirror1D::Impl::GetWidth() const
{
    return width;
}

int AlpakaDigitalRowMirror1D::Impl::GetLookupCount() const
{
    return lookupCount;
}

std::size_t AlpakaDigitalRowMirror1D::Impl::GetCellCount() const
{
    return cellCount;
}

int AlpakaDigitalRowMirror1D::Impl::GetSourceSlot() const
{
    return sourceSlot;
}

int AlpakaDigitalRowMirror1D::Impl::GetTargetSlot() const
{
    return targetSlot;
}

int AlpakaDigitalRowMirror1D::Impl::GetPastSlot() const
{
    return pastSlot;
}

void AlpakaDigitalRowMirror1D::Impl::Resize(int nextWidth, int nextLookupCount, bool nextHasPastRow)
{
    ValidateRowWidth(nextWidth);
    ValidateLookupCount(nextLookupCount);

    const std::size_t nextCellCount = CellCountForWidth(nextWidth);
    const std::size_t nextLookupCellCount = static_cast<std::size_t>(nextLookupCount);
    if (initialized && width == nextWidth && lookupCount == nextLookupCount && hasPastRow == nextHasPastRow)
    {
        dirty = true;
        return;
    }

    const Extent rowExtent = Extent{static_cast<Idx>(nextCellCount)};
    const Extent lookupExtent = Extent{static_cast<Idx>(nextLookupCellCount)};
    for (int slot = 0; slot < rowSlotCount; ++slot)
    {
        deviceRows[slot].emplace(alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, rowExtent));
        hostRows[slot].assign(nextCellCount, AlpakaDigitalValue(0));
    }
    deviceLookup.emplace(alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, lookupExtent));
    hostLookup.assign(nextLookupCellCount, AlpakaDigitalValue(0));

    width = nextWidth;
    lookupCount = nextLookupCount;
    cellCount = nextCellCount;
    lookupCellCount = nextLookupCellCount;
    initialized = true;
    dirty = true;
    hasPastRow = nextHasPastRow;
    ResetSlots();
}

void AlpakaDigitalRowMirror1D::Impl::MarkDirty()
{
    dirty = true;
}

void AlpakaDigitalRowMirror1D::Impl::CopyRowsAndLookupToDevice(const AlpakaDigitalValue *sourceRow,
    const AlpakaDigitalValue *targetRow, const AlpakaDigitalValue *pastRow, const AlpakaDigitalValue *lookup)
{
    RequireInitialized();
    PackRow(&hostRows[sourceSlot], sourceRow);
    PackRow(&hostRows[targetSlot], targetRow);
    CopyHostToDevice(&hostRows[sourceSlot], &*deviceRows[sourceSlot], cellCount);
    CopyHostToDevice(&hostRows[targetSlot], &*deviceRows[targetSlot], cellCount);
    if (hasPastRow)
    {
        PackRow(&hostRows[pastSlot], pastRow);
        CopyHostToDevice(&hostRows[pastSlot], &*deviceRows[pastSlot], cellCount);
    }
    PackLookup(lookup);
    CopyHostToDevice(&hostLookup, &*deviceLookup, lookupCellCount);
    dirty = false;
}

void AlpakaDigitalRowMirror1D::Impl::DebugCopySourceToTarget()
{
    RequireInitialized();
    CopyDeviceRowToTarget(sourceSlot);
}

void AlpakaDigitalRowMirror1D::Impl::DebugCopyPastToTarget()
{
    RequirePastRow();
    CopyDeviceRowToTarget(pastSlot);
}

void AlpakaDigitalRowMirror1D::Impl::RotateRows()
{
    RequireInitialized();
    sourceSlot = NextSlot(sourceSlot);
    targetSlot = NextSlot(targetSlot);
    if (hasPastRow)
    {
        pastSlot = NextSlot(pastSlot);
    }
}

void AlpakaDigitalRowMirror1D::Impl::CopySourceToHost(AlpakaDigitalValue *sourceRow)
{
    RequireInitialized();
    ValidateDigitalRow(sourceRow);
    CopyDeviceToHost(&*deviceRows[sourceSlot], &hostRows[sourceSlot], cellCount);
    UnpackRow(hostRows[sourceSlot], sourceRow);
}

void AlpakaDigitalRowMirror1D::Impl::CopyTargetToHost(AlpakaDigitalValue *targetRow)
{
    RequireInitialized();
    ValidateDigitalRow(targetRow);
    CopyDeviceToHost(&*deviceRows[targetSlot], &hostRows[targetSlot], cellCount);
    UnpackRow(hostRows[targetSlot], targetRow);
}

void AlpakaDigitalRowMirror1D::Impl::CopyPastToHost(AlpakaDigitalValue *pastRow)
{
    RequirePastRow();
    ValidateDigitalRow(pastRow);
    CopyDeviceToHost(&*deviceRows[pastSlot], &hostRows[pastSlot], cellCount);
    UnpackRow(hostRows[pastSlot], pastRow);
}

void AlpakaDigitalRowMirror1D::Impl::CopyLookupToHost(AlpakaDigitalValue *lookup)
{
    RequireInitialized();
    ValidateLookup(lookup);
    CopyDeviceToHost(&*deviceLookup, &hostLookup, lookupCellCount);
    UnpackLookup(lookup);
}

void AlpakaDigitalRowMirror1D::Impl::RequireInitialized() const
{
    if (!initialized)
    {
        throw std::logic_error("Alpaka digital row mirror is not initialized");
    }
}

void AlpakaDigitalRowMirror1D::Impl::RequirePastRow() const
{
    RequireInitialized();
    if (!hasPastRow)
    {
        throw std::logic_error("Alpaka digital past row is not enabled");
    }
}

void AlpakaDigitalRowMirror1D::Impl::ResetSlots()
{
    sourceSlot = initialSourceSlot;
    targetSlot = initialTargetSlot;
    pastSlot = initialPastSlot;
}

void AlpakaDigitalRowMirror1D::Impl::PackRow(
    std::vector<AlpakaDigitalValue> *hostRow, const AlpakaDigitalValue *row) const
{
    ValidateDigitalRow(row);
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        (*hostRow)[index] = row[index];
    }
}

void AlpakaDigitalRowMirror1D::Impl::PackLookup(const AlpakaDigitalValue *lookup)
{
    ValidateLookup(lookup);
    for (std::size_t index = 0; index < lookupCellCount; ++index)
    {
        hostLookup[index] = lookup[index];
    }
}

void AlpakaDigitalRowMirror1D::Impl::UnpackRow(
    const std::vector<AlpakaDigitalValue> &hostRow, AlpakaDigitalValue *row) const
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        row[index] = hostRow[index];
    }
}

void AlpakaDigitalRowMirror1D::Impl::UnpackLookup(AlpakaDigitalValue *lookup) const
{
    for (std::size_t index = 0; index < lookupCellCount; ++index)
    {
        lookup[index] = hostLookup[index];
    }
}

void AlpakaDigitalRowMirror1D::Impl::CopyHostToDevice(
    std::vector<AlpakaDigitalValue> *hostBuffer, DigitalBuffer *deviceBuffer, std::size_t valueCount)
{
    const Extent extent = Extent{static_cast<Idx>(valueCount)};
    HostDigitalView hostView = alpaka::createView(hostDevice, hostBuffer->data(), extent);
    alpaka::memcpy(queue, *deviceBuffer, hostView, extent);
    alpaka::wait(queue);
}

void AlpakaDigitalRowMirror1D::Impl::CopyDeviceToHost(
    DigitalBuffer *deviceBuffer, std::vector<AlpakaDigitalValue> *hostBuffer, std::size_t valueCount)
{
    const Extent extent = Extent{static_cast<Idx>(valueCount)};
    HostDigitalView hostView = alpaka::createView(hostDevice, hostBuffer->data(), extent);
    alpaka::memcpy(queue, hostView, *deviceBuffer, extent);
    alpaka::wait(queue);
}

void AlpakaDigitalRowMirror1D::Impl::CopyDeviceRowToTarget(int sourceRowSlot)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    alpaka::memcpy(queue, *deviceRows[targetSlot], *deviceRows[sourceRowSlot], extent);
    alpaka::wait(queue);
}

AlpakaDigitalRowMirror1D::AlpakaDigitalRowMirror1D() :
    impl(std::make_unique<Impl>())
{
}

AlpakaDigitalRowMirror1D::~AlpakaDigitalRowMirror1D() = default;

bool AlpakaDigitalRowMirror1D::IsInitialized() const
{
    return impl->IsInitialized();
}

bool AlpakaDigitalRowMirror1D::IsDirty() const
{
    return impl->IsDirty();
}

bool AlpakaDigitalRowMirror1D::HasPastRow() const
{
    return impl->HasPastRow();
}

int AlpakaDigitalRowMirror1D::GetWidth() const
{
    return impl->GetWidth();
}

int AlpakaDigitalRowMirror1D::GetLookupCount() const
{
    return impl->GetLookupCount();
}

std::size_t AlpakaDigitalRowMirror1D::GetCellCount() const
{
    return impl->GetCellCount();
}

int AlpakaDigitalRowMirror1D::GetSourceSlot() const
{
    return impl->GetSourceSlot();
}

int AlpakaDigitalRowMirror1D::GetTargetSlot() const
{
    return impl->GetTargetSlot();
}

int AlpakaDigitalRowMirror1D::GetPastSlot() const
{
    return impl->GetPastSlot();
}

void AlpakaDigitalRowMirror1D::Resize(int width, int lookupCount, bool hasPastRow)
{
    impl->Resize(width, lookupCount, hasPastRow);
}

void AlpakaDigitalRowMirror1D::MarkDirty()
{
    impl->MarkDirty();
}

void AlpakaDigitalRowMirror1D::CopyRowsAndLookupToDevice(const AlpakaDigitalValue *sourceRow,
    const AlpakaDigitalValue *targetRow, const AlpakaDigitalValue *pastRow, const AlpakaDigitalValue *lookup)
{
    impl->CopyRowsAndLookupToDevice(sourceRow, targetRow, pastRow, lookup);
}

void AlpakaDigitalRowMirror1D::DebugCopySourceToTarget()
{
    impl->DebugCopySourceToTarget();
}

void AlpakaDigitalRowMirror1D::DebugCopyPastToTarget()
{
    impl->DebugCopyPastToTarget();
}

void AlpakaDigitalRowMirror1D::RotateRows()
{
    impl->RotateRows();
}

void AlpakaDigitalRowMirror1D::CopySourceToHost(AlpakaDigitalValue *sourceRow)
{
    impl->CopySourceToHost(sourceRow);
}

void AlpakaDigitalRowMirror1D::CopyTargetToHost(AlpakaDigitalValue *targetRow)
{
    impl->CopyTargetToHost(targetRow);
}

void AlpakaDigitalRowMirror1D::CopyPastToHost(AlpakaDigitalValue *pastRow)
{
    impl->CopyPastToHost(pastRow);
}

void AlpakaDigitalRowMirror1D::CopyLookupToHost(AlpakaDigitalValue *lookup)
{
    impl->CopyLookupToHost(lookup);
}

} // namespace capow
