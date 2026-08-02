#include "AlpakaBuffers.hpp"

#include <alpaka/alpaka.hpp>

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
using PlaneBuffer = alpaka::Buf<AccPlatform, capow::AlpakaPlaneValue, Dim, Idx>;

void ValidateDimensions(int width, int height)
{
    if (width <= 0 || height <= 0)
    {
        throw std::invalid_argument("Alpaka 2D plane dimensions must be positive");
    }
}

void ValidatePlane(const capow::AlpakaPlaneValue *plane, int valueStride)
{
    if (plane == 0)
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

} // namespace capow
