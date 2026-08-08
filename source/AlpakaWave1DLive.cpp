#include "AlpakaWave1DLive.hpp"

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
using HostPlaneView = alpaka::ViewPlainPtr<HostDevice, capow::AlpakaPlaneValue, MemDim, Idx>;
using HostColorView = alpaka::ViewPlainPtr<HostDevice, std::uint32_t, MemDim, Idx>;
using PlaneBuffer = alpaka::Buf<AccPlatform, capow::AlpakaPlaneValue, MemDim, Idx>;
using PixelBuffer = alpaka::Buf<AccPlatform, std::uint32_t, MemDim, Idx>;
using ColorBuffer = alpaka::Buf<AccPlatform, std::uint32_t, MemDim, Idx>;
using FlagBuffer = alpaka::Buf<AccPlatform, std::uint8_t, MemDim, Idx>;

struct Wave1DLiveKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, const capow::AlpakaPlaneValue *source,
        const capow::AlpakaPlaneValue *past, const capow::AlpakaPlaneValue *sourceVelocity,
        const capow::AlpakaPlaneValue *frictionTweaks, const capow::AlpakaPlaneValue *springTweaks,
        const capow::AlpakaPlaneValue *massTweaks, const capow::AlpakaPlaneValue *nonlinearityTweaks,
        capow::AlpakaPlaneValue *targetIntensity, capow::AlpakaPlaneValue *targetVelocity,
        capow::AlpakaPlaneValue *targetNonlinearityTweaks, std::uint8_t *zeroFlags, Idx width,
        capow::AlpakaPlaneValue waveSpeed2TimeStep2OverDx2, capow::AlpakaPlaneValue dtOverDx2,
        capow::AlpakaPlaneValue maxIntensity, capow::AlpakaPlaneValue maxVelocity, capow::AlpakaPlaneValue timeStep,
        capow::AlpakaPlaneValue dtOverMass, capow::AlpakaPlaneValue frictionMultiplier,
        capow::AlpakaPlaneValue springMultiplier, capow::AlpakaPlaneValue driverValue,
        capow::AlpakaPlaneValue nonlinearity1, capow::AlpakaPlaneValue nonlinearity2, capow::Wave1DRule rule,
        capow::Wave1DLiveRuleFamily family, capow::Heat1DRule heatRule, capow::AlpakaPlaneValue heatIncrement) const
    {
        const Idx x = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0];
        if (x >= width)
        {
            return;
        }

        capow::Wave1DResult<capow::AlpakaPlaneValue> result = {capow::AlpakaPlaneValue(0), capow::AlpakaPlaneValue(0)};
        targetNonlinearityTweaks[x] = nonlinearityTweaks[x];
        zeroFlags[x] = 0U;
        if (family == capow::WAVE_1D_LIVE_RULE_FAMILY_HEAT)
        {
            const capow::Heat1DResult<capow::AlpakaPlaneValue> heatResult =
                heatRule == capow::HEAT_1D_RULE_FIVE_NEIGHBOR
                ? capow::ComputeHeat1D5Cell(source, x, width, heatIncrement, maxIntensity, timeStep)
                : capow::ComputeHeat1DCell(
                      source, x, width, dtOverDx2, heatIncrement, maxIntensity, maxVelocity, timeStep);
            targetIntensity[x] = heatResult.nextIntensity;
            targetVelocity[x] = heatResult.velocity;
            return;
        }
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
        else if (rule == capow::WAVE_1D_RULE_OSCILLATOR_WAVE)
        {
            const Idx leftX = x == 0U ? width - 1U : x - 1U;
            const Idx rightX = x + 1U == width ? 0U : x + 1U;
            result = capow::ComputeOscillatorWave1D<capow::AlpakaPlaneValue>(source[leftX], source[x], source[rightX],
                sourceVelocity[x], dtOverMass, frictionMultiplier, springMultiplier, driverValue, dtOverDx2,
                maxIntensity, maxVelocity, timeStep);
        }
        else if (rule == capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR_WAVE)
        {
            const Idx leftX = x == 0U ? width - 1U : x - 1U;
            const Idx rightX = x + 1U == width ? 0U : x + 1U;
            result = capow::ComputeDiverseOscillatorWave1D<capow::AlpakaPlaneValue>(source[leftX], source[x],
                source[rightX], sourceVelocity[x], dtOverMass, frictionMultiplier, springMultiplier, driverValue,
                frictionTweaks[x], springTweaks[x], massTweaks[x], dtOverDx2, maxIntensity, maxVelocity, timeStep);
        }
        else if (rule == capow::WAVE_1D_RULE_ULAM)
        {
            const Idx leftX = x == 0U ? width - 1U : x - 1U;
            const Idx rightX = x + 1U == width ? 0U : x + 1U;
            result = capow::ComputeUlamWave1D<capow::AlpakaPlaneValue>(source[leftX], source[x], source[rightX],
                past[x], waveSpeed2TimeStep2OverDx2, nonlinearity1, maxIntensity, timeStep);
        }
        else if (rule == capow::WAVE_1D_RULE_AUTO_ULAM)
        {
            const Idx leftX = x == 0U ? width - 1U : x - 1U;
            const Idx rightX = x + 1U == width ? 0U : x + 1U;
            const capow::StableUlam1DResult<capow::AlpakaPlaneValue> stableResult =
                capow::ComputeStableUlamWave1D<capow::AlpakaPlaneValue>(source[leftX], source[x], source[rightX],
                    sourceVelocity[x], nonlinearityTweaks[x], dtOverDx2, timeStep, nonlinearity2, maxIntensity,
                    maxVelocity);
            result = {stableResult.nextIntensity, stableResult.velocity};
            targetNonlinearityTweaks[x] = stableResult.nextTweak;
            zeroFlags[x] = stableResult.zeroNeighbors ? 1U : 0U;
        }
        else
        {
            const Idx leftX = x == 0U ? width - 1U : x - 1U;
            const Idx rightX = x + 1U == width ? 0U : x + 1U;
            result = capow::ComputeCubicUlamWave1D<capow::AlpakaPlaneValue>(source[leftX], source[x], source[rightX],
                past[x], waveSpeed2TimeStep2OverDx2, nonlinearity2, maxIntensity, timeStep);
        }
        targetIntensity[x] = result.nextIntensity;
        targetVelocity[x] = result.velocity;
    }
};

ALPAKA_FN_HOST_ACC Idx Wave1DUpdateOrder(Idx x, Idx width)
{
    if (x == 0U)
    {
        return width - 2U;
    }
    if (x + 1U == width)
    {
        return width - 1U;
    }
    return x - 1U;
}

struct StableUlamTweakMaskKernel
{
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc, capow::AlpakaPlaneValue *targetNonlinearityTweaks,
        const std::uint8_t *zeroFlags, Idx width) const
    {
        const Idx x = alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0];
        if (x >= width)
        {
            return;
        }

        const Idx leftX = x == 0U ? width - 1U : x - 1U;
        const Idx rightX = x + 1U == width ? 0U : x + 1U;
        const Idx xOrder = Wave1DUpdateOrder(x, width);
        if ((Wave1DUpdateOrder(leftX, width) > xOrder && zeroFlags[leftX] != 0U) ||
            (Wave1DUpdateOrder(rightX, width) > xOrder && zeroFlags[rightX] != 0U))
        {
            targetNonlinearityTweaks[x] = capow::AlpakaPlaneValue(0);
        }
    }
};

std::uint32_t DivideRoundUp(std::uint32_t value, std::uint32_t divisor)
{
    return (value + divisor - 1U) / divisor;
}

bool IsSupportedWaveRule(capow::Wave1DRule rule)
{
    return rule == capow::WAVE_1D_RULE_OSCILLATOR || rule == capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR ||
        rule == capow::WAVE_1D_RULE_OSCILLATOR_WAVE || rule == capow::WAVE_1D_RULE_DIVERSE_OSCILLATOR_WAVE ||
        rule == capow::WAVE_1D_RULE_ULAM || rule == capow::WAVE_1D_RULE_AUTO_ULAM ||
        rule == capow::WAVE_1D_RULE_CUBIC_ULAM;
}

bool IsSupportedHeatRule(capow::Heat1DRule rule)
{
    return rule == capow::HEAT_1D_RULE_THREE_NEIGHBOR || rule == capow::HEAT_1D_RULE_FIVE_NEIGHBOR;
}

void ValidateOptions(const capow::Wave1DLiveOptions &options)
{
    if (options.width <= 0 || options.historyWidth <= 0 || options.historyHeight <= 0)
    {
        throw std::invalid_argument("live CA_WAVE_1D dimensions must be positive");
    }
    if (options.width > options.historyWidth)
    {
        throw std::invalid_argument("live CA_WAVE_1D width exceeds display width");
    }
    if (options.row < 0 || options.row >= options.historyHeight)
    {
        throw std::invalid_argument("live CA_WAVE_1D row is outside display");
    }
    if (options.bltLines <= 0)
    {
        throw std::invalid_argument("live CA_WAVE_1D blt lines must be positive");
    }
    if (options.family == capow::WAVE_1D_LIVE_RULE_FAMILY_HEAT && !IsSupportedHeatRule(options.heatRule))
    {
        throw std::invalid_argument("live CA_WAVE_1D heat rule is unsupported");
    }
    if (options.family == capow::WAVE_1D_LIVE_RULE_FAMILY_WAVE && !IsSupportedWaveRule(options.rule))
    {
        throw std::invalid_argument("live CA_WAVE_1D rule is unsupported");
    }
    if (options.maxIntensity <= capow::AlpakaPlaneValue(0) || options.maxVelocity <= capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("live CA_WAVE_1D ranges must be positive");
    }
    if (options.timeStep == capow::AlpakaPlaneValue(0))
    {
        throw std::invalid_argument("live CA_WAVE_1D time step must not be zero");
    }
    if (options.colorCount <= 0)
    {
        throw std::invalid_argument("live CA_WAVE_1D color count must be positive");
    }
}

void ValidatePlane(const capow::AlpakaPlaneValue *plane, int stride)
{
    if (plane == nullptr)
    {
        throw std::invalid_argument("live CA_WAVE_1D plane pointer must not be null");
    }
    if (stride <= 0)
    {
        throw std::invalid_argument("live CA_WAVE_1D stride must be positive");
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

__global__ void ClearWave1DPixels(std::uint32_t *pixels, int count)
{
    const int index = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    if (index < count)
    {
        pixels[index] = 0U;
    }
}

__device__ int ColorIndexForWaveValue(float intensity, float velocity, float maxIntensity, float maxVelocity,
    float velocityColorScale, int colorCount, int showVelocity)
{
    const float displayValue = showVelocity ? velocityColorScale * velocity : intensity;
    const float displayRange = showVelocity ? maxVelocity : maxIntensity;
    const float scaled = static_cast<float>(colorCount - 1) * (displayValue + displayRange) / (2.0F * displayRange);
    if (scaled < 0.0F)
    {
        return 0;
    }
    if (scaled > static_cast<float>(colorCount - 1))
    {
        return colorCount - 1;
    }
    return static_cast<int>(scaled);
}

__global__ void UpdateWave1DHistoryPixels(std::uint32_t *targetPixels, const std::uint32_t *sourcePixels,
    const capow::AlpakaPlaneValue *intensity, const capow::AlpakaPlaneValue *velocity, const std::uint32_t *colors,
    int width, int historyWidth, int historyHeight, int row, int bltLines, int scroll,
    capow::AlpakaPlaneValue maxIntensity, capow::AlpakaPlaneValue maxVelocity,
    capow::AlpakaPlaneValue velocityColorScale, int colorCount, int showVelocity)
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
        const int colorIndex = ColorIndexForWaveValue(
            intensity[x], velocity[x], maxIntensity, maxVelocity, velocityColorScale, colorCount, showVelocity);
        pixel = colors[colorIndex];
    }
    targetPixels[index] = pixel;
}

__global__ void CopyWave1DPixelsToTexture(
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

class Wave1DLiveState::Impl
{
public:
    Impl();
    ~Impl();

    bool IsActive() const;
    bool NeedsSource(const Wave1DLiveOptions &nextOptions) const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool DownloadCurrentAndPast(AlpakaPlaneValue *targetIntensity, int intensityStride,
        AlpakaPlaneValue *targetVelocity, int velocityStride, AlpakaPlaneValue *pastIntensity, int pastStride,
        AlpakaPlaneValue *nonlinearityTweaks, int nonlinearityStride, std::string *error);
    bool RunFrame(const Wave1DLiveOptions &nextOptions, const AlpakaPlaneValue *sourceIntensity, int intensityStride,
        const AlpakaPlaneValue *pastIntensity, int pastStride, const AlpakaPlaneValue *sourceVelocity,
        int velocityStride, const AlpakaPlaneValue *frictionTweaks, const AlpakaPlaneValue *springTweaks,
        const AlpakaPlaneValue *massTweaks, const AlpakaPlaneValue *nonlinearityTweaks, const std::uint32_t *colorTable,
        std::string *error);

private:
    void Resize(const Wave1DLiveOptions &nextOptions);
    void ReleaseTexture();
    void EnsureTexture();
    void ClearPixels();
    void UploadSource(const AlpakaPlaneValue *sourceIntensity, int intensityStride,
        const AlpakaPlaneValue *pastIntensity, int pastStride, const AlpakaPlaneValue *sourceVelocity,
        int velocityStride);
    void UploadTweaks(const AlpakaPlaneValue *frictionTweaks, const AlpakaPlaneValue *springTweaks,
        const AlpakaPlaneValue *massTweaks, const AlpakaPlaneValue *nonlinearityTweaks);
    void UploadColors(const std::uint32_t *colorTable);
    void RunStep();
    bool UpdateTexture(std::string *error);
    void CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostPlane, PlaneBuffer *devicePlane);
    void CopyDeviceToHost(PlaneBuffer *devicePlane, std::vector<AlpakaPlaneValue> *hostPlane);

    AccDevice accDevice;
    HostDevice hostDevice;
    Queue queue;
    Wave1DLiveOptions options;
    bool initialized;
    bool active;
    std::size_t cellCount;
    std::size_t pixelCount;
    std::vector<AlpakaPlaneValue> hostCurrent;
    std::vector<AlpakaPlaneValue> hostPast;
    std::vector<AlpakaPlaneValue> hostVelocity;
    std::vector<AlpakaPlaneValue> hostFrictionTweaks;
    std::vector<AlpakaPlaneValue> hostSpringTweaks;
    std::vector<AlpakaPlaneValue> hostMassTweaks;
    std::vector<AlpakaPlaneValue> hostNonlinearityTweaks;
    std::vector<std::uint32_t> hostColors;
    std::optional<PlaneBuffer> deviceCurrent;
    std::optional<PlaneBuffer> devicePast;
    std::optional<PlaneBuffer> deviceNextIntensity;
    std::optional<PlaneBuffer> deviceCurrentVelocity;
    std::optional<PlaneBuffer> deviceNextVelocity;
    std::optional<PlaneBuffer> deviceFrictionTweaks;
    std::optional<PlaneBuffer> deviceSpringTweaks;
    std::optional<PlaneBuffer> deviceMassTweaks;
    std::optional<PlaneBuffer> deviceNonlinearityTweaks;
    std::optional<PlaneBuffer> deviceNextNonlinearityTweaks;
    std::optional<FlagBuffer> deviceZeroFlags;
    std::optional<PixelBuffer> devicePixels;
    std::optional<PixelBuffer> deviceNextPixels;
    std::optional<ColorBuffer> deviceColors;
    unsigned int texture;
    cudaGraphicsResource *textureResource;
};

Wave1DLiveOptions::Wave1DLiveOptions() :
    width(0),
    historyWidth(0),
    historyHeight(0),
    row(0),
    bltLines(1),
    view(WAVE_1D_LIVE_VIEW_SCROLL),
    family(WAVE_1D_LIVE_RULE_FAMILY_WAVE),
    rule(WAVE_1D_RULE_OSCILLATOR),
    heatRule(HEAT_1D_RULE_THREE_NEIGHBOR),
    waveSpeed2TimeStep2OverDx2(AlpakaPlaneValue(0)),
    dtOverDx2(AlpakaPlaneValue(0)),
    heatIncrement(AlpakaPlaneValue(0)),
    maxIntensity(AlpakaPlaneValue(0)),
    maxVelocity(AlpakaPlaneValue(0)),
    timeStep(AlpakaPlaneValue(0)),
    dtOverMass(AlpakaPlaneValue(0)),
    frictionMultiplier(AlpakaPlaneValue(0)),
    springMultiplier(AlpakaPlaneValue(0)),
    driverValue(AlpakaPlaneValue(0)),
    nonlinearity1(AlpakaPlaneValue(0)),
    nonlinearity2(AlpakaPlaneValue(0)),
    velocityColorScale(AlpakaPlaneValue(1)),
    colorCount(0),
    showVelocity(false)
{
}

Wave1DLiveState::Impl::Impl() :
    accDevice(alpaka::getDevByIdx(AccPlatform{}, 0U)),
    hostDevice(alpaka::getDevByIdx(HostPlatform{}, 0U)),
    queue(accDevice),
    initialized(false),
    active(false),
    cellCount(0U),
    pixelCount(0U),
    texture(0U),
    textureResource(nullptr)
{
}

Wave1DLiveState::Impl::~Impl()
{
    ReleaseTexture();
}

bool Wave1DLiveState::Impl::IsActive() const
{
    return active;
}

bool Wave1DLiveState::Impl::NeedsSource(const Wave1DLiveOptions &nextOptions) const
{
    return !active || !initialized || options.width != nextOptions.width ||
        options.historyWidth != nextOptions.historyWidth || options.historyHeight != nextOptions.historyHeight ||
        options.colorCount != nextOptions.colorCount || options.family != nextOptions.family ||
        options.rule != nextOptions.rule || options.heatRule != nextOptions.heatRule;
}

unsigned int Wave1DLiveState::Impl::GetTexture() const
{
    return texture;
}

void Wave1DLiveState::Impl::Deactivate()
{
    active = false;
}

bool Wave1DLiveState::Impl::DownloadCurrentAndPast(AlpakaPlaneValue *targetIntensity, int intensityStride,
    AlpakaPlaneValue *targetVelocity, int velocityStride, AlpakaPlaneValue *pastIntensity, int pastStride,
    AlpakaPlaneValue *nonlinearityTweaks, int nonlinearityStride, std::string *error)
{
    try
    {
        ValidatePlane(targetIntensity, intensityStride);
        ValidatePlane(targetVelocity, velocityStride);
        ValidatePlane(pastIntensity, pastStride);
        ValidatePlane(nonlinearityTweaks, nonlinearityStride);
        if (!active || !deviceCurrent || !deviceCurrentVelocity || !devicePast || !deviceNonlinearityTweaks)
        {
            return true;
        }

        CopyDeviceToHost(&*deviceCurrent, &hostCurrent);
        CopyDeviceToHost(&*deviceCurrentVelocity, &hostVelocity);
        CopyDeviceToHost(&*devicePast, &hostPast);
        CopyDeviceToHost(&*deviceNonlinearityTweaks, &hostNonlinearityTweaks);
        for (std::size_t index = 0; index < cellCount; ++index)
        {
            targetIntensity[index * static_cast<std::size_t>(intensityStride)] = hostCurrent[index];
            targetVelocity[index * static_cast<std::size_t>(velocityStride)] = hostVelocity[index];
            pastIntensity[index * static_cast<std::size_t>(pastStride)] = hostPast[index];
            nonlinearityTweaks[index * static_cast<std::size_t>(nonlinearityStride)] = hostNonlinearityTweaks[index];
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

bool Wave1DLiveState::Impl::RunFrame(const Wave1DLiveOptions &nextOptions, const AlpakaPlaneValue *sourceIntensity,
    int intensityStride, const AlpakaPlaneValue *pastIntensity, int pastStride, const AlpakaPlaneValue *sourceVelocity,
    int velocityStride, const AlpakaPlaneValue *frictionTweaks, const AlpakaPlaneValue *springTweaks,
    const AlpakaPlaneValue *massTweaks, const AlpakaPlaneValue *nonlinearityTweaks, const std::uint32_t *colorTable,
    std::string *error)
{
    try
    {
        ValidateOptions(nextOptions);
        if (colorTable == nullptr)
        {
            throw std::invalid_argument("live CA_WAVE_1D color table must not be null");
        }
        const bool needsSource = NeedsSource(nextOptions);
        Resize(nextOptions);
        EnsureTexture();
        if (needsSource)
        {
            ValidatePlane(sourceIntensity, intensityStride);
            ValidatePlane(pastIntensity, pastStride);
            ValidatePlane(sourceVelocity, velocityStride);
            ValidatePlane(frictionTweaks, 1);
            ValidatePlane(springTweaks, 1);
            ValidatePlane(massTweaks, 1);
            ValidatePlane(nonlinearityTweaks, 1);
            UploadSource(sourceIntensity, intensityStride, pastIntensity, pastStride, sourceVelocity, velocityStride);
            UploadTweaks(frictionTweaks, springTweaks, massTweaks, nonlinearityTweaks);
        }
        UploadColors(colorTable);
        RunStep();
        if (!UpdateTexture(error))
        {
            return false;
        }
        std::swap(devicePast, deviceCurrent);
        std::swap(deviceCurrent, deviceNextIntensity);
        std::swap(deviceCurrentVelocity, deviceNextVelocity);
        std::swap(deviceNonlinearityTweaks, deviceNextNonlinearityTweaks);
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

void Wave1DLiveState::Impl::Resize(const Wave1DLiveOptions &nextOptions)
{
    const bool sameCellSize = initialized && options.width == nextOptions.width;
    const bool sameDisplaySize = initialized && options.historyWidth == nextOptions.historyWidth &&
        options.historyHeight == nextOptions.historyHeight && options.colorCount == nextOptions.colorCount;
    options = nextOptions;
    if (sameCellSize && sameDisplaySize)
    {
        return;
    }

    cellCount = static_cast<std::size_t>(options.width);
    pixelCount = static_cast<std::size_t>(options.historyWidth) * static_cast<std::size_t>(options.historyHeight);
    const Extent cellExtent = Extent{static_cast<Idx>(cellCount)};
    const Extent pixelExtent = Extent{static_cast<Idx>(pixelCount)};
    const Extent colorExtent = Extent{static_cast<Idx>(options.colorCount)};
    deviceCurrent.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    devicePast.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceNextIntensity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceCurrentVelocity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceNextVelocity.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceFrictionTweaks.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceSpringTweaks.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceMassTweaks.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceNonlinearityTweaks.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceNextNonlinearityTweaks.emplace(alpaka::allocBuf<AlpakaPlaneValue, Idx>(accDevice, cellExtent));
    deviceZeroFlags.emplace(alpaka::allocBuf<std::uint8_t, Idx>(accDevice, cellExtent));
    devicePixels.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, pixelExtent));
    deviceNextPixels.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, pixelExtent));
    deviceColors.emplace(alpaka::allocBuf<std::uint32_t, Idx>(accDevice, colorExtent));
    hostCurrent.assign(cellCount, AlpakaPlaneValue(0));
    hostPast.assign(cellCount, AlpakaPlaneValue(0));
    hostVelocity.assign(cellCount, AlpakaPlaneValue(0));
    hostFrictionTweaks.assign(cellCount, AlpakaPlaneValue(1));
    hostSpringTweaks.assign(cellCount, AlpakaPlaneValue(1));
    hostMassTweaks.assign(cellCount, AlpakaPlaneValue(1));
    hostNonlinearityTweaks.assign(cellCount, AlpakaPlaneValue(1));
    hostColors.assign(static_cast<std::size_t>(options.colorCount), 0U);
    initialized = true;
    active = false;
    ClearPixels();
    ReleaseTexture();
}

void Wave1DLiveState::Impl::ReleaseTexture()
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

void Wave1DLiveState::Impl::EnsureTexture()
{
    if (texture != 0U && textureResource != nullptr)
    {
        return;
    }

    glGenTextures(1, &texture);
    if (texture == 0U)
    {
        throw std::runtime_error("could not create live CA_WAVE_1D OpenGL texture");
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

void Wave1DLiveState::Impl::ClearPixels()
{
    if (!devicePixels || !deviceNextPixels || pixelCount == 0U)
    {
        return;
    }

    const int count = static_cast<int>(pixelCount);
    const int threads = 256;
    const int blocks = static_cast<int>(DivideRoundUp(static_cast<std::uint32_t>(count), threads));
    ClearWave1DPixels<<<blocks, threads>>>(alpaka::getPtrNative(*devicePixels), count);
    ClearWave1DPixels<<<blocks, threads>>>(alpaka::getPtrNative(*deviceNextPixels), count);
    cudaDeviceSynchronize();
}

void Wave1DLiveState::Impl::UploadSource(const AlpakaPlaneValue *sourceIntensity, int intensityStride,
    const AlpakaPlaneValue *pastIntensity, int pastStride, const AlpakaPlaneValue *sourceVelocity, int velocityStride)
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        hostCurrent[index] = sourceIntensity[index * static_cast<std::size_t>(intensityStride)];
        hostPast[index] = pastIntensity[index * static_cast<std::size_t>(pastStride)];
        hostVelocity[index] = sourceVelocity[index * static_cast<std::size_t>(velocityStride)];
    }
    CopyHostToDevice(&hostCurrent, &*deviceCurrent);
    CopyHostToDevice(&hostPast, &*devicePast);
    CopyHostToDevice(&hostVelocity, &*deviceCurrentVelocity);
}

void Wave1DLiveState::Impl::UploadTweaks(const AlpakaPlaneValue *frictionTweaks, const AlpakaPlaneValue *springTweaks,
    const AlpakaPlaneValue *massTweaks, const AlpakaPlaneValue *nonlinearityTweaks)
{
    for (std::size_t index = 0; index < cellCount; ++index)
    {
        hostFrictionTweaks[index] = frictionTweaks[index];
        hostSpringTweaks[index] = springTweaks[index];
        hostMassTweaks[index] = massTweaks[index];
        hostNonlinearityTweaks[index] = nonlinearityTweaks[index];
    }
    CopyHostToDevice(&hostFrictionTweaks, &*deviceFrictionTweaks);
    CopyHostToDevice(&hostSpringTweaks, &*deviceSpringTweaks);
    CopyHostToDevice(&hostMassTweaks, &*deviceMassTweaks);
    CopyHostToDevice(&hostNonlinearityTweaks, &*deviceNonlinearityTweaks);
}

void Wave1DLiveState::Impl::UploadColors(const std::uint32_t *colorTable)
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

void Wave1DLiveState::Impl::RunStep()
{
    const Extent threads = Extent{128U};
    const Extent blocks = Extent{DivideRoundUp(static_cast<std::uint32_t>(options.width), threads[0])};
    const Extent elements = Extent::all(1U);
    const WorkDiv workDiv = WorkDiv{blocks, threads, elements};
    const Wave1DLiveKernel kernel = Wave1DLiveKernel{};

    alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, kernel, alpaka::getPtrNative(*deviceCurrent),
        alpaka::getPtrNative(*devicePast), alpaka::getPtrNative(*deviceCurrentVelocity),
        alpaka::getPtrNative(*deviceFrictionTweaks), alpaka::getPtrNative(*deviceSpringTweaks),
        alpaka::getPtrNative(*deviceMassTweaks), alpaka::getPtrNative(*deviceNonlinearityTweaks),
        alpaka::getPtrNative(*deviceNextIntensity), alpaka::getPtrNative(*deviceNextVelocity),
        alpaka::getPtrNative(*deviceNextNonlinearityTweaks), alpaka::getPtrNative(*deviceZeroFlags),
        static_cast<Idx>(options.width), options.waveSpeed2TimeStep2OverDx2, options.dtOverDx2, options.maxIntensity,
        options.maxVelocity, options.timeStep, options.dtOverMass, options.frictionMultiplier, options.springMultiplier,
        options.driverValue, options.nonlinearity1, options.nonlinearity2, options.rule, options.family,
        options.heatRule, options.heatIncrement);
    if (options.rule == WAVE_1D_RULE_AUTO_ULAM)
    {
        const StableUlamTweakMaskKernel maskKernel = StableUlamTweakMaskKernel{};
        alpaka::exec<alpaka::TagGpuCudaRt>(queue, workDiv, maskKernel,
            alpaka::getPtrNative(*deviceNextNonlinearityTweaks), alpaka::getPtrNative(*deviceZeroFlags),
            static_cast<Idx>(options.width));
    }
    alpaka::wait(queue);
}

bool Wave1DLiveState::Impl::UpdateTexture(std::string *error)
{
    const bool scroll =
        options.view == WAVE_1D_LIVE_VIEW_SCROLL && options.row == options.historyHeight - options.bltLines;
    const int threads = 256;
    const int blocks = static_cast<int>(DivideRoundUp(static_cast<std::uint32_t>(pixelCount), threads));
    UpdateWave1DHistoryPixels<<<blocks, threads>>>(alpaka::getPtrNative(*deviceNextPixels),
        alpaka::getPtrNative(*devicePixels), alpaka::getPtrNative(*deviceNextIntensity),
        alpaka::getPtrNative(*deviceNextVelocity), alpaka::getPtrNative(*deviceColors), options.width,
        options.historyWidth, options.historyHeight, options.row, options.bltLines, scroll ? 1 : 0,
        options.maxIntensity, options.maxVelocity, options.velocityColorScale, options.colorCount,
        options.showVelocity ? 1 : 0);
    const cudaError_t historyLaunch = cudaGetLastError();
    const cudaError_t historySync = cudaDeviceSynchronize();
    if (!SetCudaError(historyLaunch, "launch live CA_WAVE_1D history kernel", error) ||
        !SetCudaError(historySync, "sync live CA_WAVE_1D history kernel", error))
    {
        return false;
    }
    std::swap(devicePixels, deviceNextPixels);

    if (!SetCudaError(cudaGraphicsMapResources(1, &textureResource, 0), "map live CA_WAVE_1D texture", error))
    {
        return false;
    }

    cudaArray_t textureArray = 0;
    if (!SetCudaError(cudaGraphicsSubResourceGetMappedArray(&textureArray, textureResource, 0, 0),
            "get live CA_WAVE_1D texture array", error))
    {
        cudaGraphicsUnmapResources(1, &textureResource, 0);
        return false;
    }

    cudaResourceDesc resourceDesc;
    memset(&resourceDesc, 0, sizeof(resourceDesc));
    resourceDesc.resType = cudaResourceTypeArray;
    resourceDesc.res.array.array = textureArray;

    cudaSurfaceObject_t surface = 0;
    if (!SetCudaError(cudaCreateSurfaceObject(&surface, &resourceDesc), "create live CA_WAVE_1D surface", error))
    {
        cudaGraphicsUnmapResources(1, &textureResource, 0);
        return false;
    }

    const dim3 textureThreads(16U, 16U);
    const dim3 textureBlocks(DivideRoundUp(static_cast<std::uint32_t>(options.historyWidth), textureThreads.x),
        DivideRoundUp(static_cast<std::uint32_t>(options.historyHeight), textureThreads.y));
    CopyWave1DPixelsToTexture<<<textureBlocks, textureThreads>>>(
        surface, alpaka::getPtrNative(*devicePixels), options.historyWidth, options.historyHeight);

    const cudaError_t textureLaunch = cudaGetLastError();
    const cudaError_t textureSync = cudaDeviceSynchronize();
    cudaDestroySurfaceObject(surface);
    cudaGraphicsUnmapResources(1, &textureResource, 0);
    return SetCudaError(textureLaunch, "launch live CA_WAVE_1D texture kernel", error) &&
        SetCudaError(textureSync, "sync live CA_WAVE_1D texture kernel", error);
}

void Wave1DLiveState::Impl::CopyHostToDevice(std::vector<AlpakaPlaneValue> *hostPlane, PlaneBuffer *devicePlane)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    HostPlaneView hostView = alpaka::createView(hostDevice, hostPlane->data(), extent);
    alpaka::memcpy(queue, *devicePlane, hostView, extent);
    alpaka::wait(queue);
}

void Wave1DLiveState::Impl::CopyDeviceToHost(PlaneBuffer *devicePlane, std::vector<AlpakaPlaneValue> *hostPlane)
{
    const Extent extent = Extent{static_cast<Idx>(cellCount)};
    HostPlaneView hostView = alpaka::createView(hostDevice, hostPlane->data(), extent);
    alpaka::memcpy(queue, hostView, *devicePlane, extent);
    alpaka::wait(queue);
}

Wave1DLiveState::Wave1DLiveState() :
    impl(std::make_unique<Impl>())
{
}

Wave1DLiveState::~Wave1DLiveState() = default;

bool Wave1DLiveState::IsActive() const
{
    return impl->IsActive();
}

bool Wave1DLiveState::NeedsSource(const Wave1DLiveOptions &options) const
{
    return impl->NeedsSource(options);
}

unsigned int Wave1DLiveState::GetTexture() const
{
    return impl->GetTexture();
}

void Wave1DLiveState::Deactivate()
{
    impl->Deactivate();
}

bool Wave1DLiveState::DownloadCurrentAndPast(AlpakaPlaneValue *targetIntensity, int intensityStride,
    AlpakaPlaneValue *targetVelocity, int velocityStride, AlpakaPlaneValue *pastIntensity, int pastStride,
    AlpakaPlaneValue *nonlinearityTweaks, int nonlinearityStride, std::string *error)
{
    return impl->DownloadCurrentAndPast(targetIntensity, intensityStride, targetVelocity, velocityStride, pastIntensity,
        pastStride, nonlinearityTweaks, nonlinearityStride, error);
}

bool Wave1DLiveState::RunFrame(const Wave1DLiveOptions &options, const AlpakaPlaneValue *sourceIntensity,
    int intensityStride, const AlpakaPlaneValue *pastIntensity, int pastStride, const AlpakaPlaneValue *sourceVelocity,
    int velocityStride, const AlpakaPlaneValue *frictionTweaks, const AlpakaPlaneValue *springTweaks,
    const AlpakaPlaneValue *massTweaks, const AlpakaPlaneValue *nonlinearityTweaks, const std::uint32_t *colorTable,
    std::string *error)
{
    return impl->RunFrame(options, sourceIntensity, intensityStride, pastIntensity, pastStride, sourceVelocity,
        velocityStride, frictionTweaks, springTweaks, massTweaks, nonlinearityTweaks, colorTable, error);
}

} // namespace capow
