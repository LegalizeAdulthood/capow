#include "AlpakaHeat2DLive.hpp"

#include "AlpakaUtilities.hpp"
#include "CapowRules.hpp"

#include <alpaka/alpaka.hpp>
#include <cuda_runtime.h>

#include <GL/gl.h>
#include <Windows.h>

#include <cuda_gl_interop.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <vector>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

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
using HostPlaneView = alpaka::ViewPlainPtr<HostDevice, capow::AlpakaPlaneValue, MemDim, Idx>;
using HostColorView = alpaka::ViewPlainPtr<HostDevice, std::uint32_t, MemDim, Idx>;
using PlaneBuffer = alpaka::Buf<AccPlatform, capow::AlpakaPlaneValue, MemDim, Idx>;
using ColorBuffer = alpaka::Buf<AccPlatform, std::uint32_t, MemDim, Idx>;

struct Heat2DLiveKernel
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

__device__ int ColorIndexForValue(float value, float maxIntensity, int colorCount)
{
    const float scaled = static_cast<float>(colorCount - 1) * (value + maxIntensity) / (2.0F * maxIntensity);
    if (scaled < 0.0F)
    {
        return colorCount - 1;
    }
    if (scaled > static_cast<float>(colorCount - 1))
    {
        return colorCount - 1;
    }
    return static_cast<int>(scaled);
}

__global__ void ColorizeHeat2DTexture(cudaSurfaceObject_t surface, const capow::AlpakaPlaneValue *targetIntensity,
    const capow::AlpakaPlaneValue *targetVelocity, const std::uint32_t *colors, int width, int height,
    capow::AlpakaPlaneValue maxIntensity, capow::AlpakaPlaneValue velocityColorScale, int colorCount, int showVelocity)
{
    const int x = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int y = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
    if (x >= width || y >= height)
    {
        return;
    }

    const int index = y * width + x;
    const capow::AlpakaPlaneValue displayValue =
        showVelocity ? velocityColorScale * targetVelocity[index] : targetIntensity[index];
    const std::uint32_t color = colors[ColorIndexForValue(displayValue, maxIntensity, colorCount)];
    const uchar4 pixel = make_uchar4(static_cast<unsigned char>(color & 0xFFU),
        static_cast<unsigned char>((color >> 8U) & 0xFFU), static_cast<unsigned char>((color >> 16U) & 0xFFU), 255U);
    surf2Dwrite(pixel, surface, x * static_cast<int>(sizeof(uchar4)), y);
}

std::size_t CellCount(int width, int height)
{
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
}

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor)
{
    return (value + divisor - 1U) / divisor;
}

void ValidateOptions(const capow::Heat2DLiveOptions &options)
{
    if (options.width <= 0 || options.height <= 0)
    {
        throw std::invalid_argument("live CA_HEAT_2D dimensions must be positive");
    }
    if (options.maxIntensity <= capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("live CA_HEAT_2D max intensity must be positive");
    }
    if (options.timeStep == capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("live CA_HEAT_2D time step must not be zero");
    }
    if (options.colorCount <= 0)
    {
        throw std::invalid_argument("live CA_HEAT_2D color count must be positive");
    }
}

void ValidatePlane(const capow::AlpakaPlaneValue *plane, int valueStride)
{
    if (plane == 0)
    {
        throw std::invalid_argument("live CA_HEAT_2D plane pointer must not be null");
    }
    if (valueStride <= 0)
    {
        throw std::invalid_argument("live CA_HEAT_2D plane stride must be positive");
    }
}

bool SetCudaError(cudaError_t result, const char *action, std::string *error)
{
    if (result == cudaSuccess)
    {
        return true;
    }
    if (error != 0)
    {
        *error = action;
        *error += ": ";
        *error += cudaGetErrorString(result);
    }
    return false;
}

} // namespace

namespace capow
{

class Heat2DLiveState::Impl
{
public:
    Impl();
    ~Impl();

    bool IsActive() const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool RunFrame(const Heat2DLiveOptions &nextOptions, const AlpakaPlaneValue *sourcePlane, int valueStride,
        const std::uint32_t *colorTable, std::string *error);

private:
    void Resize(const Heat2DLiveOptions &nextOptions);
    void ReleaseTexture();
    void EnsureTexture();
    void UploadSource(const AlpakaPlaneValue *sourcePlane, int valueStride);
    void UploadColors(const std::uint32_t *colorTable);
    void RunHeatStep();
    bool ColorizeTexture(std::string *error);
    void CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostPlane, PlaneBuffer *devicePlane);

    AccDevice accDevice;
    HostDevice hostDevice;
    Queue queue;
    Heat2DLiveOptions options;
    bool initialized;
    bool active;
    std::size_t cellCount;
    std::vector<AlpakaPlaneValue> hostSource;
    std::vector<std::uint32_t> hostColors;
    std::optional<PlaneBuffer> deviceCurrent;
    std::optional<PlaneBuffer> deviceNextIntensity;
    std::optional<PlaneBuffer> deviceVelocity;
    std::optional<ColorBuffer> deviceColors;
    unsigned int texture;
    cudaGraphicsResource *textureResource;
};

Heat2DLiveState::Impl::Impl() :
    accDevice(alpaka::getDevByIdx(AccPlatform{}, 0U)),
    hostDevice(alpaka::getDevByIdx(HostPlatform{}, 0U)),
    queue(accDevice),
    options{0, 0, AlpakaPlaneValue(0), AlpakaPlaneValue(0), AlpakaPlaneValue(0), AlpakaPlaneValue(0), 0, false},
    initialized(false),
    active(false),
    cellCount(0U),
    texture(0U),
    textureResource(0)
{
}

Heat2DLiveState::Impl::~Impl()
{
    ReleaseTexture();
}

bool Heat2DLiveState::Impl::IsActive() const
{
    return active;
}

unsigned int Heat2DLiveState::Impl::GetTexture() const
{
    return texture;
}

void Heat2DLiveState::Impl::Deactivate()
{
    active = false;
}

bool Heat2DLiveState::Impl::RunFrame(const Heat2DLiveOptions &nextOptions, const AlpakaPlaneValue *sourcePlane,
    int valueStride, const std::uint32_t *colorTable, std::string *error)
{
    try
    {
        ValidateOptions(nextOptions);
        ValidatePlane(sourcePlane, valueStride);
        if (colorTable == 0)
        {
            throw std::invalid_argument("live CA_HEAT_2D color table must not be null");
        }
        Resize(nextOptions);
        EnsureTexture();
        if (!active)
        {
            UploadSource(sourcePlane, valueStride);
        }
        UploadColors(colorTable);
        RunHeatStep();
        if (!ColorizeTexture(error))
        {
            return false;
        }
        std::swap(deviceCurrent, deviceNextIntensity);
        active = true;
        return true;
    }
    catch (const std::exception &exception)
    {
        if (error != 0)
        {
            *error = exception.what();
        }
        return false;
    }
}

void Heat2DLiveState::Impl::Resize(const Heat2DLiveOptions &nextOptions)
{
    const bool sameSize = initialized && options.width == nextOptions.width && options.height == nextOptions.height;
    const bool sameColorCount = initialized && options.colorCount == nextOptions.colorCount;
    options = nextOptions;
    if (sameSize && sameColorCount)
    {
        return;
    }

    cellCount = CellCount(options.width, options.height);
    const MemExtent planeExtent = MemExtent{static_cast<Idx>(cellCount)};
    const MemExtent colorExtent = MemExtent{static_cast<Idx>(options.colorCount)};
    deviceCurrent.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, planeExtent));
    deviceNextIntensity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, planeExtent));
    deviceVelocity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, planeExtent));
    deviceColors.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, colorExtent));
    hostSource.assign(cellCount, AlpakaPlaneValue(0));
    hostColors.assign(static_cast<std::size_t>(options.colorCount), 0U);
    active = false;
    initialized = true;
    ReleaseTexture();
}

void Heat2DLiveState::Impl::ReleaseTexture()
{
    if (textureResource != 0)
    {
        cudaGraphicsUnregisterResource(textureResource);
        textureResource = 0;
    }
    if (texture != 0U)
    {
        if (wglGetCurrentContext() != NULL)
            glDeleteTextures(1, &texture);
        texture = 0U;
    }
}

void Heat2DLiveState::Impl::EnsureTexture()
{
    if (texture != 0U && textureResource != 0)
    {
        return;
    }

    glGenTextures(1, &texture);
    if (texture == 0U)
    {
        throw std::runtime_error("could not create live CA_HEAT_2D OpenGL texture");
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, options.width, options.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    const cudaError_t result = cudaGraphicsGLRegisterImage(
        &textureResource, texture, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsSurfaceLoadStore);
    if (result != cudaSuccess)
    {
        glDeleteTextures(1, &texture);
        texture = 0U;
        throw std::runtime_error(cudaGetErrorString(result));
    }
}

void Heat2DLiveState::Impl::UploadSource(const AlpakaPlaneValue *sourcePlane, int valueStride)
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        hostSource[index] = sourcePlane[index * static_cast<std::size_t>(valueStride)];
    }
    CopyHostToDevice(&hostSource, &*deviceCurrent);
}

void Heat2DLiveState::Impl::UploadColors(const std::uint32_t *colorTable)
{
    for (int index = 0; index < options.colorCount; ++index)
    {
        hostColors[static_cast<std::size_t>(index)] = colorTable[index];
    }

    const MemExtent colorExtent = MemExtent{static_cast<Idx>(options.colorCount)};
    HostColorView hostView = alpaka::createView(hostDevice, hostColors.data(), colorExtent);
    alpaka::memcpy(queue, *deviceColors, hostView, colorExtent);
    alpaka::wait(queue);
}

void Heat2DLiveState::Impl::RunHeatStep()
{
    const WorkExtent threads = WorkExtent{16U, 16U};
    const WorkExtent blocks = WorkExtent{DivideRoundUp(static_cast<std::uint32_t>(options.height), threads[0]),
        DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[1])};
    const WorkExtent elements = WorkExtent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const Heat2DLiveKernel kernel = Heat2DLiveKernel{};

    alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(*deviceCurrent),
        alpaka::getPtrNative(*deviceNextIntensity), alpaka::getPtrNative(*deviceVelocity),
        static_cast<Idx>(options.width), static_cast<Idx>(options.height), options.heatIncrement, options.maxIntensity,
        options.timeStep);
    alpaka::wait(queue);
}

bool Heat2DLiveState::Impl::ColorizeTexture(std::string *error)
{
    if (!SetCudaError(cudaGraphicsMapResources(1, &textureResource, 0), "map live CA_HEAT_2D texture", error))
    {
        return false;
    }

    cudaArray_t textureArray = 0;
    if (!SetCudaError(cudaGraphicsSubResourceGetMappedArray(&textureArray, textureResource, 0, 0),
            "get live CA_HEAT_2D texture array", error))
    {
        cudaGraphicsUnmapResources(1, &textureResource, 0);
        return false;
    }

    cudaResourceDesc resourceDesc;
    memset(&resourceDesc, 0, sizeof(resourceDesc));
    resourceDesc.resType = cudaResourceTypeArray;
    resourceDesc.res.array.array = textureArray;

    cudaSurfaceObject_t surface = 0;
    if (!SetCudaError(cudaCreateSurfaceObject(&surface, &resourceDesc), "create live CA_HEAT_2D surface", error))
    {
        cudaGraphicsUnmapResources(1, &textureResource, 0);
        return false;
    }

    const dim3 threads(16U, 16U);
    const dim3 blocks(DivideRoundUp(static_cast<std::uint32_t>(options.width), threads.x),
        DivideRoundUp(static_cast<std::uint32_t>(options.height), threads.y));
    ColorizeHeat2DTexture<<<blocks, threads>>>(surface, alpaka::getPtrNative(*deviceNextIntensity),
        alpaka::getPtrNative(*deviceVelocity), alpaka::getPtrNative(*deviceColors), options.width, options.height,
        options.maxIntensity, options.velocityColorScale, options.colorCount, options.showVelocity ? 1 : 0);

    const cudaError_t launchResult = cudaGetLastError();
    const cudaError_t syncResult = cudaDeviceSynchronize();
    cudaDestroySurfaceObject(surface);
    cudaGraphicsUnmapResources(1, &textureResource, 0);
    return SetCudaError(launchResult, "launch live CA_HEAT_2D color kernel", error) &&
        SetCudaError(syncResult, "sync live CA_HEAT_2D color kernel", error);
}

void Heat2DLiveState::Impl::CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostPlane, PlaneBuffer *devicePlane)
{
    const MemExtent extent = MemExtent{static_cast<Idx>(cellCount)};
    HostPlaneView hostView = alpaka::createView(hostDevice, hostPlane->data(), extent);
    alpaka::memcpy(queue, *devicePlane, hostView, extent);
    alpaka::wait(queue);
}

Heat2DLiveState::Heat2DLiveState() :
    impl(std::make_unique<Impl>())
{
}

Heat2DLiveState::~Heat2DLiveState() = default;

bool Heat2DLiveState::IsActive() const
{
    return impl->IsActive();
}

unsigned int Heat2DLiveState::GetTexture() const
{
    return impl->GetTexture();
}

void Heat2DLiveState::Deactivate()
{
    impl->Deactivate();
}

bool Heat2DLiveState::RunFrame(const Heat2DLiveOptions &options, const AlpakaPlaneValue *sourcePlane, int valueStride,
    const std::uint32_t *colorTable, std::string *error)
{
    return impl->RunFrame(options, sourcePlane, valueStride, colorTable, error);
}

} // namespace capow
