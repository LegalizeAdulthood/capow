#include "AlpakaHeat2DLive.hpp"

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
        capow::AlpakaPlaneValue heatIncrement, capow::AlpakaPlaneValue maxIntensity, capow::AlpakaPlaneValue timeStep,
        capow::Heat2DBoundaryMode boundaryMode) const
    {
        const alpaka::Vec<WorkDim, Idx> global = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc);
        const Idx y = global[0];
        const Idx x = global[1];

        if (x >= width || y >= height)
        {
            return;
        }

        const capow::Heat2DResult<capow::AlpakaPlaneValue> result = capow::ComputeHeat2DCell<capow::AlpakaPlaneValue>(
            source, x, y, width, height, boundaryMode, heatIncrement, maxIntensity, timeStep);
        const Idx center = capow::Heat2DIndex(x, y, width);
        targetIntensity[center] = result.nextIntensity;
        targetVelocity[center] = result.velocity;
    }
};

struct Wave2DLiveKernel
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
    if (plane == nullptr)
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
    if (error != nullptr)
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
    bool NeedsSource(const Heat2DLiveOptions &nextOptions) const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool DownloadCurrent(AlpakaPlaneValue *targetPlane, int valueStride, AlpakaPlaneValue *targetVelocity,
        int velocityStride, std::string *error);
    bool DownloadCurrentAndPast(AlpakaPlaneValue *targetPlane, int valueStride, AlpakaPlaneValue *targetVelocity,
        int velocityStride, AlpakaPlaneValue *pastPlane, int pastStride, std::string *error);
    bool RunFrame(const Heat2DLiveOptions &nextOptions, const AlpakaPlaneValue *sourcePlane, int valueStride,
        const std::uint32_t *colorTable, std::string *error);
    bool RunFrame(const Heat2DLiveOptions &nextOptions, const AlpakaPlaneValue *sourcePlane, int valueStride,
        const AlpakaPlaneValue *pastPlane, int pastStride, const std::uint32_t *colorTable, std::string *error);

private:
    void Resize(const Heat2DLiveOptions &nextOptions);
    void ReleaseTexture();
    void EnsureTexture();
    void UploadSource(const AlpakaPlaneValue *sourcePlane, int valueStride);
    void UploadPast(const AlpakaPlaneValue *pastPlane, int pastStride);
    void UploadColors(const std::uint32_t *colorTable);
    void RunHeatStep();
    void RunWaveStep();
    bool ColorizeTexture(std::string *error);
    void CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostPlane, PlaneBuffer *devicePlane);
    void CopyDeviceToHost(PlaneBuffer *devicePlane, std::vector<AlpakaPlaneValue> *hostPlane);

    AccDevice accDevice;
    HostDevice hostDevice;
    Queue queue;
    Heat2DLiveOptions options;
    bool initialized;
    bool active;
    std::size_t cellCount;
    std::vector<AlpakaPlaneValue> hostSource;
    std::vector<AlpakaPlaneValue> hostPast;
    std::vector<AlpakaPlaneValue> hostVelocity;
    std::vector<std::uint32_t> hostColors;
    bool colorTableUploaded;
    std::optional<PlaneBuffer> deviceCurrent;
    std::optional<PlaneBuffer> devicePast;
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
    options{0, 0, LIVE_2D_RULE_HEAT, AlpakaPlaneValue(0), AlpakaPlaneValue(0), AlpakaPlaneValue(0), AlpakaPlaneValue(0),
        AlpakaPlaneValue(0), 0, false, HEAT_2D_BOUNDARY_WRAP},
    initialized(false),
    active(false),
    cellCount(0U),
    colorTableUploaded(false),
    texture(0U),
    textureResource(nullptr)
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

bool Heat2DLiveState::Impl::NeedsSource(const Heat2DLiveOptions &nextOptions) const
{
    return !active || !initialized || options.width != nextOptions.width || options.height != nextOptions.height ||
        options.colorCount != nextOptions.colorCount || options.rule != nextOptions.rule;
}

unsigned int Heat2DLiveState::Impl::GetTexture() const
{
    return texture;
}

void Heat2DLiveState::Impl::Deactivate()
{
    active = false;
}

bool Heat2DLiveState::Impl::DownloadCurrent(AlpakaPlaneValue *targetPlane, int valueStride,
    AlpakaPlaneValue *targetVelocity, int velocityStride, std::string *error)
{
    return DownloadCurrentAndPast(targetPlane, valueStride, targetVelocity, velocityStride, nullptr, 0, error);
}

bool Heat2DLiveState::Impl::DownloadCurrentAndPast(AlpakaPlaneValue *targetPlane, int valueStride,
    AlpakaPlaneValue *targetVelocity, int velocityStride, AlpakaPlaneValue *pastPlane, int pastStride,
    std::string *error)
{
    try
    {
        ValidatePlane(targetPlane, valueStride);
        ValidatePlane(targetVelocity, velocityStride);
        if (pastPlane != nullptr)
        {
            ValidatePlane(pastPlane, pastStride);
        }
        if (!active || !deviceCurrent || !deviceVelocity)
        {
            return true;
        }

        CopyDeviceToHost(&*deviceCurrent, &hostSource);
        CopyDeviceToHost(&*deviceVelocity, &hostVelocity);
        if (pastPlane != nullptr && devicePast)
        {
            CopyDeviceToHost(&*devicePast, &hostPast);
        }
        for (std::size_t index = 0; index < cellCount; ++index)
        {
            targetPlane[index * static_cast<std::size_t>(valueStride)] = hostSource[index];
            targetVelocity[index * static_cast<std::size_t>(velocityStride)] = hostVelocity[index];
            if (pastPlane != nullptr && devicePast)
            {
                pastPlane[index * static_cast<std::size_t>(pastStride)] = hostPast[index];
            }
        }
        return true;
    }
    catch (const std::exception &exception)
    {
        if (error != nullptr)
        {
            *error = exception.what();
        }
        return false;
    }
}

bool Heat2DLiveState::Impl::RunFrame(const Heat2DLiveOptions &nextOptions, const AlpakaPlaneValue *sourcePlane,
    int valueStride, const std::uint32_t *colorTable, std::string *error)
{
    return RunFrame(nextOptions, sourcePlane, valueStride, nullptr, 0, colorTable, error);
}

bool Heat2DLiveState::Impl::RunFrame(const Heat2DLiveOptions &nextOptions, const AlpakaPlaneValue *sourcePlane,
    int valueStride, const AlpakaPlaneValue *pastPlane, int pastStride, const std::uint32_t *colorTable,
    std::string *error)
{
    try
    {
        ValidateOptions(nextOptions);
        if (colorTable == nullptr)
        {
            throw std::invalid_argument("live CA_HEAT_2D color table must not be null");
        }
        const bool needsSource = NeedsSource(nextOptions);
        Resize(nextOptions);
        EnsureTexture();
        if (needsSource)
        {
            ValidatePlane(sourcePlane, valueStride);
            UploadSource(sourcePlane, valueStride);
            if (options.rule == LIVE_2D_RULE_WAVE)
            {
                ValidatePlane(pastPlane, pastStride);
                UploadPast(pastPlane, pastStride);
            }
        }
        UploadColors(colorTable);
        if (options.rule == LIVE_2D_RULE_WAVE)
            RunWaveStep();
        else
            RunHeatStep();
        if (!ColorizeTexture(error))
        {
            return false;
        }
        if (options.rule == LIVE_2D_RULE_WAVE)
            std::swap(devicePast, deviceCurrent);
        std::swap(deviceCurrent, deviceNextIntensity);
        active = true;
        return true;
    }
    catch (const std::exception &exception)
    {
        if (error != nullptr)
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
    devicePast.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, planeExtent));
    deviceNextIntensity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, planeExtent));
    deviceVelocity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, planeExtent));
    deviceColors.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, colorExtent));
    hostSource.assign(cellCount, AlpakaPlaneValue(0));
    hostPast.assign(cellCount, AlpakaPlaneValue(0));
    hostVelocity.assign(cellCount, AlpakaPlaneValue(0));
    hostColors.assign(static_cast<std::size_t>(options.colorCount), 0U);
    colorTableUploaded = false;
    active = false;
    initialized = true;
    ReleaseTexture();
}

void Heat2DLiveState::Impl::ReleaseTexture()
{
    if (textureResource != nullptr)
    {
        cudaGraphicsUnregisterResource(textureResource);
        textureResource = nullptr;
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
    if (texture != 0U && textureResource != nullptr)
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

void Heat2DLiveState::Impl::UploadPast(const AlpakaPlaneValue *pastPlane, int pastStride)
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        hostPast[index] = pastPlane[index * static_cast<std::size_t>(pastStride)];
    }
    CopyHostToDevice(&hostPast, &*devicePast);
}

void Heat2DLiveState::Impl::UploadColors(const std::uint32_t *colorTable)
{
    if (colorTableUploaded && std::equal(hostColors.begin(), hostColors.end(), colorTable))
        return;

    for (int index = 0; index < options.colorCount; ++index)
    {
        hostColors[static_cast<std::size_t>(index)] = colorTable[index];
    }

    const MemExtent colorExtent = MemExtent{static_cast<Idx>(options.colorCount)};
    HostColorView hostView = alpaka::createView(hostDevice, hostColors.data(), colorExtent);
    alpaka::memcpy(queue, *deviceColors, hostView, colorExtent);
    alpaka::wait(queue);
    colorTableUploaded = true;
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
        options.timeStep, options.boundaryMode);
    alpaka::wait(queue);
}

void Heat2DLiveState::Impl::RunWaveStep()
{
    const WorkExtent threads = WorkExtent{16U, 16U};
    const WorkExtent blocks = WorkExtent{DivideRoundUp(static_cast<std::uint32_t>(options.height), threads[0]),
        DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[1])};
    const WorkExtent elements = WorkExtent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const Wave2DLiveKernel kernel = Wave2DLiveKernel{};

    alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(*deviceCurrent),
        alpaka::getPtrNative(*devicePast), alpaka::getPtrNative(*deviceNextIntensity),
        alpaka::getPtrNative(*deviceVelocity), static_cast<Idx>(options.width), static_cast<Idx>(options.height),
        options.waveSpeed2TimeStep2OverDx2, options.maxIntensity, options.timeStep);
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

void Heat2DLiveState::Impl::CopyDeviceToHost(PlaneBuffer *devicePlane, std::vector<AlpakaPlaneValue> *hostPlane)
{
    const MemExtent extent = MemExtent{static_cast<Idx>(cellCount)};
    HostPlaneView hostView = alpaka::createView(hostDevice, hostPlane->data(), extent);
    alpaka::memcpy(queue, hostView, *devicePlane, extent);
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

bool Heat2DLiveState::NeedsSource(const Heat2DLiveOptions &options) const
{
    return impl->NeedsSource(options);
}

unsigned int Heat2DLiveState::GetTexture() const
{
    return impl->GetTexture();
}

void Heat2DLiveState::Deactivate()
{
    impl->Deactivate();
}

bool Heat2DLiveState::DownloadCurrent(AlpakaPlaneValue *targetPlane, int valueStride, AlpakaPlaneValue *targetVelocity,
    int velocityStride, std::string *error)
{
    return impl->DownloadCurrent(targetPlane, valueStride, targetVelocity, velocityStride, error);
}

bool Heat2DLiveState::DownloadCurrentAndPast(AlpakaPlaneValue *targetPlane, int valueStride,
    AlpakaPlaneValue *targetVelocity, int velocityStride, AlpakaPlaneValue *pastPlane, int pastStride,
    std::string *error)
{
    return impl->DownloadCurrentAndPast(
        targetPlane, valueStride, targetVelocity, velocityStride, pastPlane, pastStride, error);
}

bool Heat2DLiveState::RunFrame(const Heat2DLiveOptions &options, const AlpakaPlaneValue *sourcePlane, int valueStride,
    const std::uint32_t *colorTable, std::string *error)
{
    return impl->RunFrame(options, sourcePlane, valueStride, colorTable, error);
}

bool Heat2DLiveState::RunFrame(const Heat2DLiveOptions &options, const AlpakaPlaneValue *sourcePlane, int valueStride,
    const AlpakaPlaneValue *pastPlane, int pastStride, const std::uint32_t *colorTable, std::string *error)
{
    return impl->RunFrame(options, sourcePlane, valueStride, pastPlane, pastStride, colorTable, error);
}

} // namespace capow
