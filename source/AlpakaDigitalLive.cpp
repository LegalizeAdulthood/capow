#include "AlpakaDigitalLive.hpp"

#include "CapowRules.hpp"

#include <alpaka/alpaka.hpp>
#include <cuda_runtime.h>

#include <GL/gl.h>
#include <Windows.h>

#include <cuda_gl_interop.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#ifndef GL_RGBA8
#define GL_RGBA8 0x8058
#endif

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
using HostColorView = alpaka::ViewPlainPtr<HostDevice, std::uint32_t, MemDim, Idx>;
using DigitalBuffer = alpaka::Buf<AccPlatform, capow::AlpakaDigitalValue, MemDim, Idx>;
using PixelBuffer = alpaka::Buf<AccPlatform, std::uint32_t, MemDim, Idx>;
using ColorBuffer = alpaka::Buf<AccPlatform, std::uint32_t, MemDim, Idx>;

struct StandardDigitalLiveKernel
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

void ValidateOptions(const capow::Digital1DLiveOptions &options)
{
    if (options.width <= 2 * options.radius || options.historyWidth <= 0 || options.historyHeight <= 0)
    {
        throw std::invalid_argument("live CA_STANDARD dimensions must be positive");
    }
    if (options.width > options.historyWidth)
    {
        throw std::invalid_argument("live CA_STANDARD width exceeds display width");
    }
    if (options.row < 0 || options.row >= options.historyHeight)
    {
        throw std::invalid_argument("live CA_STANDARD row is outside display");
    }
    if (options.bltLines <= 0)
    {
        throw std::invalid_argument("live CA_STANDARD blt lines must be positive");
    }
    if (options.radius <= 0 || options.stateBits <= 0 || options.stateBits > 8)
    {
        throw std::invalid_argument("live CA_STANDARD radius or state bits are invalid");
    }
    if (options.lookupCount <= 0 || options.colorCount <= 0)
    {
        throw std::invalid_argument("live CA_STANDARD lookup and color counts must be positive");
    }
}

void ValidateDigitalRow(const capow::AlpakaDigitalValue *row)
{
    if (row == nullptr)
    {
        throw std::invalid_argument("live CA_STANDARD row pointer must not be null");
    }
}

void ValidateColorTable(const std::uint32_t *colorTable)
{
    if (colorTable == nullptr)
    {
        throw std::invalid_argument("live CA_STANDARD color table must not be null");
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

__global__ void ClearDigital1DPixels(std::uint32_t *pixels, int count)
{
    const int index = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    if (index < count)
    {
        pixels[index] = 0U;
    }
}

__device__ int ColorIndexForDigitalValue(unsigned char value, int colorCount)
{
    const int colorIndex = static_cast<int>(value);
    if (colorIndex < 0)
    {
        return 0;
    }
    if (colorIndex >= colorCount)
    {
        return colorCount - 1;
    }
    return colorIndex;
}

__global__ void UpdateDigital1DHistoryPixels(std::uint32_t *targetPixels, const std::uint32_t *sourcePixels,
    const capow::AlpakaDigitalValue *stateRow, const std::uint32_t *colors, int width, int historyWidth,
    int historyHeight, int row, int bltLines, int scroll, int colorCount)
{
    const int index = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int pixelCount = historyWidth * historyHeight;
    if (index >= pixelCount)
    {
        return;
    }

    const int y = index / historyWidth;
    const int x = index - y * historyWidth;
    std::uint32_t pixel = 0U;
    if (scroll)
    {
        const int sourceY = y + bltLines;
        if (sourceY < historyHeight)
        {
            pixel = sourcePixels[sourceY * historyWidth + x];
        }
    }
    else
    {
        pixel = sourcePixels[index];
    }

    if (y == row && x < width)
    {
        pixel = colors[ColorIndexForDigitalValue(stateRow[x], colorCount)];
    }
    targetPixels[index] = pixel;
}

__global__ void CopyDigital1DPixelsToTexture(
    cudaSurfaceObject_t surface, const std::uint32_t *pixels, int width, int height)
{
    const int x = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int y = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
    if (x >= width || y >= height)
    {
        return;
    }

    const std::uint32_t color = pixels[y * width + x];
    const uchar4 pixel = make_uchar4(static_cast<unsigned char>(color & 0xFFU),
        static_cast<unsigned char>((color >> 8U) & 0xFFU), static_cast<unsigned char>((color >> 16U) & 0xFFU), 255U);
    surf2Dwrite(pixel, surface, x * static_cast<int>(sizeof(uchar4)), y);
}

} // namespace

namespace capow
{

class Digital1DLiveState::Impl
{
public:
    Impl();
    ~Impl();

    bool IsActive() const;
    bool NeedsSource(const Digital1DLiveOptions &nextOptions) const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool DownloadCurrent(AlpakaDigitalValue *targetRow, std::string *error);
    bool RunFrame(const Digital1DLiveOptions &nextOptions, const AlpakaDigitalValue *sourceRow,
        const AlpakaDigitalValue *lookup, const std::uint32_t *colorTable, std::string *error);

private:
    void Resize(const Digital1DLiveOptions &nextOptions);
    void ReleaseTexture();
    void EnsureTexture();
    void ClearPixels();
    void UploadSource(const AlpakaDigitalValue *sourceRow);
    void UploadLookup(const AlpakaDigitalValue *lookup);
    void UploadColors(const std::uint32_t *colorTable);
    void RunStep();
    bool UpdateTexture(std::string *error);
    void CopyHostToDevice(std::vector<AlpakaDigitalValue> *hostRow, DigitalBuffer *deviceRow, std::size_t valueCount);
    void CopyDeviceToHost(DigitalBuffer *deviceRow, std::vector<AlpakaDigitalValue> *hostRow);

    AccDevice accDevice;
    HostDevice hostDevice;
    Queue queue;
    Digital1DLiveOptions options;
    bool initialized;
    bool active;
    std::size_t cellCount;
    std::size_t lookupCellCount;
    std::size_t pixelCount;
    std::vector<AlpakaDigitalValue> hostCurrent;
    std::vector<AlpakaDigitalValue> hostLookup;
    std::vector<std::uint32_t> hostColors;
    std::optional<DigitalBuffer> deviceCurrent;
    std::optional<DigitalBuffer> deviceNext;
    std::optional<DigitalBuffer> deviceLookup;
    std::optional<PixelBuffer> devicePixels;
    std::optional<PixelBuffer> deviceNextPixels;
    std::optional<ColorBuffer> deviceColors;
    unsigned int texture;
    cudaGraphicsResource *textureResource;
};

Digital1DLiveOptions::Digital1DLiveOptions() :
    width(0),
    historyWidth(0),
    historyHeight(0),
    row(0),
    bltLines(1),
    view(DIGITAL_1D_LIVE_VIEW_SCROLL),
    radius(1),
    stateBits(1),
    lookupCount(0),
    colorCount(0)
{
}

Digital1DLiveState::Impl::Impl() :
    accDevice(alpaka::getDevByIdx(AccPlatform{}, 0U)),
    hostDevice(alpaka::getDevByIdx(HostPlatform{}, 0U)),
    queue(accDevice),
    initialized(false),
    active(false),
    cellCount(0U),
    lookupCellCount(0U),
    pixelCount(0U),
    texture(0U),
    textureResource(nullptr)
{
}

Digital1DLiveState::Impl::~Impl()
{
    ReleaseTexture();
}

bool Digital1DLiveState::Impl::IsActive() const
{
    return active;
}

bool Digital1DLiveState::Impl::NeedsSource(const Digital1DLiveOptions &nextOptions) const
{
    return !active || !initialized || options.width != nextOptions.width ||
        options.historyWidth != nextOptions.historyWidth || options.historyHeight != nextOptions.historyHeight ||
        options.colorCount != nextOptions.colorCount || options.radius != nextOptions.radius ||
        options.stateBits != nextOptions.stateBits || options.lookupCount != nextOptions.lookupCount ||
        options.view != nextOptions.view;
}

unsigned int Digital1DLiveState::Impl::GetTexture() const
{
    return texture;
}

void Digital1DLiveState::Impl::Deactivate()
{
    active = false;
}

bool Digital1DLiveState::Impl::DownloadCurrent(AlpakaDigitalValue *targetRow, std::string *error)
{
    try
    {
        ValidateDigitalRow(targetRow);
        if (!active || !deviceCurrent)
        {
            return true;
        }

        CopyDeviceToHost(&*deviceCurrent, &hostCurrent);
        for (std::size_t index = 0; index < cellCount; ++index)
        {
            targetRow[index] = hostCurrent[index];
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

bool Digital1DLiveState::Impl::RunFrame(const Digital1DLiveOptions &nextOptions, const AlpakaDigitalValue *sourceRow,
    const AlpakaDigitalValue *lookup, const std::uint32_t *colorTable, std::string *error)
{
    try
    {
        ValidateOptions(nextOptions);
        ValidateColorTable(colorTable);
        const bool needsSource = NeedsSource(nextOptions);
        Resize(nextOptions);
        EnsureTexture();
        if (needsSource)
        {
            ValidateDigitalRow(sourceRow);
            ValidateDigitalRow(lookup);
            UploadSource(sourceRow);
            UploadLookup(lookup);
        }
        UploadColors(colorTable);
        RunStep();
        if (!UpdateTexture(error))
        {
            return false;
        }
        std::swap(deviceCurrent, deviceNext);
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

void Digital1DLiveState::Impl::Resize(const Digital1DLiveOptions &nextOptions)
{
    const bool sameCellSize = initialized && options.width == nextOptions.width;
    const bool sameLookupSize = initialized && options.lookupCount == nextOptions.lookupCount;
    const bool sameDisplaySize = initialized && options.historyWidth == nextOptions.historyWidth &&
        options.historyHeight == nextOptions.historyHeight && options.colorCount == nextOptions.colorCount;
    const bool sameView = initialized && options.view == nextOptions.view;
    options = nextOptions;
    if (sameCellSize && sameLookupSize && sameDisplaySize && sameView)
    {
        return;
    }

    cellCount = static_cast<std::size_t>(options.width);
    lookupCellCount = static_cast<std::size_t>(options.lookupCount);
    pixelCount = static_cast<std::size_t>(options.historyWidth) * static_cast<std::size_t>(options.historyHeight);
    const Extent cellExtent = Extent{static_cast<Idx>(cellCount)};
    const Extent lookupExtent = Extent{static_cast<Idx>(lookupCellCount)};
    const Extent pixelExtent = Extent{static_cast<Idx>(pixelCount)};
    const Extent colorExtent = Extent{static_cast<Idx>(options.colorCount)};
    deviceCurrent.emplace(alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, cellExtent));
    deviceNext.emplace(alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, cellExtent));
    deviceLookup.emplace(alpaka::allocBuf<AlpakaDigitalValue, Idx>(accDevice, lookupExtent));
    devicePixels.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, pixelExtent));
    deviceNextPixels.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, pixelExtent));
    deviceColors.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, colorExtent));
    hostCurrent.assign(cellCount, AlpakaDigitalValue(0));
    hostLookup.assign(lookupCellCount, AlpakaDigitalValue(0));
    hostColors.assign(static_cast<std::size_t>(options.colorCount), 0U);
    initialized = true;
    active = false;
    ClearPixels();
    ReleaseTexture();
}

void Digital1DLiveState::Impl::ReleaseTexture()
{
    if (textureResource != nullptr)
    {
        cudaGraphicsUnregisterResource(textureResource);
        textureResource = nullptr;
    }
    if (texture != 0U)
    {
        if (wglGetCurrentContext() != nullptr)
            glDeleteTextures(1, &texture);
        texture = 0U;
    }
}

void Digital1DLiveState::Impl::EnsureTexture()
{
    if (texture != 0U && textureResource != nullptr)
    {
        return;
    }

    glGenTextures(1, &texture);
    if (texture == 0U)
    {
        throw std::runtime_error("could not create live CA_STANDARD OpenGL texture");
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, options.historyWidth, options.historyHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
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

void Digital1DLiveState::Impl::ClearPixels()
{
    if (!devicePixels || !deviceNextPixels || pixelCount == 0U)
    {
        return;
    }

    const int count = static_cast<int>(pixelCount);
    const int threads = 256;
    const int blocks = static_cast<int>(DivideRoundUp(static_cast<std::uint32_t>(count), threads));
    ClearDigital1DPixels<<<blocks, threads>>>(alpaka::getPtrNative(*devicePixels), count);
    ClearDigital1DPixels<<<blocks, threads>>>(alpaka::getPtrNative(*deviceNextPixels), count);
    cudaDeviceSynchronize();
}

void Digital1DLiveState::Impl::UploadSource(const AlpakaDigitalValue *sourceRow)
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        hostCurrent[index] = sourceRow[index];
    }
    CopyHostToDevice(&hostCurrent, &*deviceCurrent, cellCount);
}

void Digital1DLiveState::Impl::UploadLookup(const AlpakaDigitalValue *lookup)
{
    for (std::size_t index = 0; index < lookupCellCount; ++index)
    {
        hostLookup[index] = lookup[index];
    }
    CopyHostToDevice(&hostLookup, &*deviceLookup, lookupCellCount);
}

void Digital1DLiveState::Impl::UploadColors(const std::uint32_t *colorTable)
{
    for (int index = 0; index < options.colorCount; ++index)
    {
        hostColors[static_cast<std::size_t>(index)] = colorTable[index];
    }

    const Extent colorExtent = Extent{static_cast<Idx>(options.colorCount)};
    HostColorView hostView = alpaka::createView(hostDevice, hostColors.data(), colorExtent);
    alpaka::memcpy(queue, *deviceColors, hostView, colorExtent);
    alpaka::wait(queue);
}

void Digital1DLiveState::Impl::RunStep()
{
    const Extent threads = Extent{128U};
    const Extent blocks = Extent{DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[0])};
    const Extent elements = Extent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const StandardDigitalLiveKernel kernel = StandardDigitalLiveKernel{};

    alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(*deviceCurrent),
        alpaka::getPtrNative(*deviceLookup), alpaka::getPtrNative(*deviceNext), static_cast<Idx>(options.width),
        static_cast<Idx>(options.radius), static_cast<Idx>(options.stateBits));
    alpaka::wait(queue);
}

bool Digital1DLiveState::Impl::UpdateTexture(std::string *error)
{
    const bool scroll =
        options.view == DIGITAL_1D_LIVE_VIEW_SCROLL && options.row == options.historyHeight - options.bltLines;
    const int threads = 256;
    const int blocks = static_cast<int>(DivideRoundUp(static_cast<std::uint32_t>(pixelCount), threads));
    UpdateDigital1DHistoryPixels<<<blocks, threads>>>(alpaka::getPtrNative(*deviceNextPixels),
        alpaka::getPtrNative(*devicePixels), alpaka::getPtrNative(*deviceNext), alpaka::getPtrNative(*deviceColors),
        options.width, options.historyWidth, options.historyHeight, options.row, options.bltLines, scroll ? 1 : 0,
        options.colorCount);
    const cudaError_t historyLaunch = cudaGetLastError();
    const cudaError_t historySync = cudaDeviceSynchronize();
    if (!SetCudaError(historyLaunch, "launch live CA_STANDARD history kernel", error) ||
        !SetCudaError(historySync, "sync live CA_STANDARD history kernel", error))
    {
        return false;
    }
    std::swap(devicePixels, deviceNextPixels);

    if (!SetCudaError(cudaGraphicsMapResources(1, &textureResource, 0), "map live CA_STANDARD texture", error))
    {
        return false;
    }

    cudaArray_t textureArray = nullptr;
    if (!SetCudaError(cudaGraphicsSubResourceGetMappedArray(&textureArray, textureResource, 0, 0),
            "get live CA_STANDARD texture array", error))
    {
        cudaGraphicsUnmapResources(1, &textureResource, 0);
        return false;
    }

    cudaResourceDesc resourceDesc;
    memset(&resourceDesc, 0, sizeof(resourceDesc));
    resourceDesc.resType = cudaResourceTypeArray;
    resourceDesc.res.array.array = textureArray;

    cudaSurfaceObject_t surface = 0;
    if (!SetCudaError(cudaCreateSurfaceObject(&surface, &resourceDesc), "create live CA_STANDARD surface", error))
    {
        cudaGraphicsUnmapResources(1, &textureResource, 0);
        return false;
    }

    const dim3 textureThreads(16U, 16U);
    const dim3 textureBlocks(DivideRoundUp(static_cast<std::uint32_t>(options.historyWidth), textureThreads.x),
        DivideRoundUp(static_cast<std::uint32_t>(options.historyHeight), textureThreads.y));
    CopyDigital1DPixelsToTexture<<<textureBlocks, textureThreads>>>(
        surface, alpaka::getPtrNative(*devicePixels), options.historyWidth, options.historyHeight);

    const cudaError_t textureLaunch = cudaGetLastError();
    const cudaError_t textureSync = cudaDeviceSynchronize();
    cudaDestroySurfaceObject(surface);
    cudaGraphicsUnmapResources(1, &textureResource, 0);
    return SetCudaError(textureLaunch, "launch live CA_STANDARD texture kernel", error) &&
        SetCudaError(textureSync, "sync live CA_STANDARD texture kernel", error);
}

void Digital1DLiveState::Impl::CopyHostToDevice(
    std::vector<AlpakaDigitalValue> *hostRow, DigitalBuffer *deviceRow, std::size_t valueCount)
{
    const Extent extent = Extent{static_cast<Idx>(valueCount)};
    HostDigitalView hostView = alpaka::createView(hostDevice, hostRow->data(), extent);
    alpaka::memcpy(queue, *deviceRow, hostView, extent);
    alpaka::wait(queue);
}

void Digital1DLiveState::Impl::CopyDeviceToHost(DigitalBuffer *deviceRow, std::vector<AlpakaDigitalValue> *hostRow)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    HostDigitalView hostView = alpaka::createView(hostDevice, hostRow->data(), extent);
    alpaka::memcpy(queue, hostView, *deviceRow, extent);
    alpaka::wait(queue);
}

Digital1DLiveState::Digital1DLiveState() :
    impl(std::make_unique<Impl>())
{
}

Digital1DLiveState::~Digital1DLiveState() = default;

bool Digital1DLiveState::IsActive() const
{
    return impl->IsActive();
}

bool Digital1DLiveState::NeedsSource(const Digital1DLiveOptions &options) const
{
    return impl->NeedsSource(options);
}

unsigned int Digital1DLiveState::GetTexture() const
{
    return impl->GetTexture();
}

void Digital1DLiveState::Deactivate()
{
    impl->Deactivate();
}

bool Digital1DLiveState::DownloadCurrent(AlpakaDigitalValue *targetRow, std::string *error)
{
    return impl->DownloadCurrent(targetRow, error);
}

bool Digital1DLiveState::RunFrame(const Digital1DLiveOptions &options, const AlpakaDigitalValue *sourceRow,
    const AlpakaDigitalValue *lookup, const std::uint32_t *colorTable, std::string *error)
{
    return impl->RunFrame(options, sourceRow, lookup, colorTable, error);
}

} // namespace capow
