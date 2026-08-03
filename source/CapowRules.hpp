#ifndef CAPOWRULES_HPP
#define CAPOWRULES_HPP

#if defined(__has_include)
#if __has_include(<alpaka/core/Common.hpp>)
#include <alpaka/core/Common.hpp>
#endif
#endif

#if !defined(ALPAKA_FN_HOST_ACC)
#define ALPAKA_FN_HOST_ACC
#endif

#include <cstdint>

namespace capow
{

enum Heat2DBoundaryMode
{
    HEAT_2D_BOUNDARY_WRAP,
    HEAT_2D_BOUNDARY_FREE,
    HEAT_2D_BOUNDARY_ABSORB,
    HEAT_2D_BOUNDARY_ZERO,
    HEAT_2D_BOUNDARY_FIXED
};

template <typename T>
struct Heat2DResult
{
    T nextIntensity;
    T velocity;
};

template <typename T>
ALPAKA_FN_HOST_ACC T WrapRange(T value, T lowValue, T highValue)
{
    if (value < lowValue)
    {
        if (value < lowValue + lowValue)
        {
            value = lowValue;
        }
        return highValue - (lowValue - value);
    }
    if (value > highValue)
    {
        if (value > highValue + highValue)
        {
            value = highValue;
        }
        return lowValue + (value - highValue);
    }
    return value;
}

template <typename T>
ALPAKA_FN_HOST_ACC Heat2DResult<T> ComputeHeat2D(T centerIntensity, T eastIntensity, T northIntensity, T westIntensity,
    T southIntensity, T heatIncrement, T maxIntensity, T timeStep)
{
    const T nabeAverage = (centerIntensity + eastIntensity + northIntensity + westIntensity + southIntensity) / T(5);
    const T nextIntensity = WrapRange(nabeAverage + heatIncrement, -maxIntensity, maxIntensity);
    return Heat2DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
}

ALPAKA_FN_HOST_ACC inline std::uint32_t Heat2DIndex(std::uint32_t x, std::uint32_t y, std::uint32_t width)
{
    return y * width + x;
}

ALPAKA_FN_HOST_ACC inline bool IsHeat2DBoundaryCell(
    std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height)
{
    return x == 0U || y == 0U || x + 1U == width || y + 1U == height;
}

ALPAKA_FN_HOST_ACC inline std::uint32_t Heat2DWrapPrevious(std::uint32_t value, std::uint32_t extent)
{
    return value == 0U ? extent - 1U : value - 1U;
}

ALPAKA_FN_HOST_ACC inline std::uint32_t Heat2DWrapNext(std::uint32_t value, std::uint32_t extent)
{
    return value + 1U == extent ? 0U : value + 1U;
}

ALPAKA_FN_HOST_ACC inline std::uint32_t Heat2DFreePrevious(std::uint32_t value)
{
    return value == 0U ? 0U : value - 1U;
}

ALPAKA_FN_HOST_ACC inline std::uint32_t Heat2DFreeNext(std::uint32_t value, std::uint32_t extent)
{
    return value + 1U == extent ? value : value + 1U;
}

ALPAKA_FN_HOST_ACC inline std::uint32_t Heat2DAbsorbIndex(
    std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height)
{
    if (width < 2U || height < 2U)
    {
        return Heat2DIndex(x, y, width);
    }
    if (y == 0U)
    {
        if (x == 0U)
        {
            return Heat2DIndex(1U, 1U, width);
        }
        if (x + 1U == width)
        {
            return Heat2DIndex(x, 1U, width);
        }
        return Heat2DIndex(x, 1U, width);
    }
    if (y + 1U == height)
    {
        if (x == 0U)
        {
            return Heat2DIndex(1U, height - 2U, width);
        }
        if (x + 1U == width)
        {
            return Heat2DIndex(width - 2U, height - 2U, width);
        }
        return Heat2DIndex(x, height - 2U, width);
    }
    if (x == 0U)
    {
        return Heat2DIndex(1U, y, width);
    }
    if (x + 1U == width)
    {
        return Heat2DIndex(width - 2U, y, width);
    }
    return Heat2DIndex(x, y, width);
}

template <typename T>
ALPAKA_FN_HOST_ACC Heat2DResult<T> ComputeHeat2DCell(const T *source, std::uint32_t x, std::uint32_t y,
    std::uint32_t width, std::uint32_t height, Heat2DBoundaryMode boundaryMode, T heatIncrement, T maxIntensity,
    T timeStep)
{
    const std::uint32_t center = Heat2DIndex(x, y, width);
    if (IsHeat2DBoundaryCell(x, y, width, height))
    {
        if (boundaryMode == HEAT_2D_BOUNDARY_ZERO)
        {
            return Heat2DResult<T>{T(0), T(0)};
        }
        if (boundaryMode == HEAT_2D_BOUNDARY_FIXED)
        {
            return Heat2DResult<T>{source[center], T(0)};
        }
        if (boundaryMode == HEAT_2D_BOUNDARY_ABSORB)
        {
            return Heat2DResult<T>{source[Heat2DAbsorbIndex(x, y, width, height)], T(0)};
        }
    }

    std::uint32_t westX;
    std::uint32_t eastX;
    std::uint32_t northY;
    std::uint32_t southY;
    if (boundaryMode == HEAT_2D_BOUNDARY_FREE)
    {
        westX = Heat2DFreePrevious(x);
        eastX = Heat2DFreeNext(x, width);
        northY = Heat2DFreePrevious(y);
        southY = Heat2DFreeNext(y, height);
    }
    else
    {
        westX = Heat2DWrapPrevious(x, width);
        eastX = Heat2DWrapNext(x, width);
        northY = Heat2DWrapPrevious(y, height);
        southY = Heat2DWrapNext(y, height);
    }

    return ComputeHeat2D<T>(source[center], source[Heat2DIndex(eastX, y, width)], source[Heat2DIndex(x, northY, width)],
        source[Heat2DIndex(westX, y, width)], source[Heat2DIndex(x, southY, width)], heatIncrement, maxIntensity,
        timeStep);
}

} // namespace capow

#endif
