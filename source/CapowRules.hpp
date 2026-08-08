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

enum Heat1DRule
{
    HEAT_1D_RULE_THREE_NEIGHBOR,
    HEAT_1D_RULE_FIVE_NEIGHBOR
};

enum Wave1DRule
{
    WAVE_1D_RULE_THREE_NEIGHBOR,
    WAVE_1D_RULE_FIVE_NEIGHBOR,
    WAVE_1D_RULE_OSCILLATOR,
    WAVE_1D_RULE_DIVERSE_OSCILLATOR,
    WAVE_1D_RULE_OSCILLATOR_WAVE,
    WAVE_1D_RULE_DIVERSE_OSCILLATOR_WAVE,
    WAVE_1D_RULE_ULAM,
    WAVE_1D_RULE_AUTO_ULAM,
    WAVE_1D_RULE_CUBIC_ULAM
};

template <typename T>
struct Heat2DResult
{
    T nextIntensity;
    T velocity;
};

template <typename T>
struct Heat1DResult
{
    T nextIntensity;
    T velocity;
};

template <typename T>
struct Wave2DResult
{
    T nextIntensity;
    T velocity;
};

template <typename T>
struct Wave1DResult
{
    T nextIntensity;
    T velocity;
};

template <typename T>
struct StableUlam1DResult
{
    T nextIntensity;
    T velocity;
    T nextTweak;
    bool zeroNeighbors;
};

template <typename T>
ALPAKA_FN_HOST_ACC T ClampRange(T value, T lowValue, T highValue)
{
    if (value < lowValue)
        return lowValue;
    if (value > highValue)
        return highValue;
    return value;
}

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
ALPAKA_FN_HOST_ACC Heat1DResult<T> ComputeHeat1D(T leftIntensity, T centerIntensity, T rightIntensity, T dtOverDx2,
    T heatIncrement, T maxIntensity, T maxVelocity, T timeStep)
{
    T nextIntensity =
        (dtOverDx2 * leftIntensity + centerIntensity + dtOverDx2 * rightIntensity) / (T(1) + T(2) * dtOverDx2) +
        timeStep * heatIncrement;
    T velocity = (nextIntensity - centerIntensity) / timeStep;
    velocity = ClampRange(velocity, -maxVelocity, maxVelocity);
    nextIntensity = WrapRange(nextIntensity, -maxIntensity, maxIntensity);
    return Heat1DResult<T>{nextIntensity, velocity};
}

template <typename T>
ALPAKA_FN_HOST_ACC Heat1DResult<T> ComputeHeat1D5(T leftLeftIntensity, T leftIntensity, T centerIntensity,
    T rightIntensity, T rightRightIntensity, T heatIncrement, T maxIntensity, T timeStep)
{
    T nextIntensity = heatIncrement +
        (leftLeftIntensity + leftIntensity + centerIntensity + rightIntensity + rightRightIntensity) / T(5);
    if (nextIntensity > maxIntensity)
    {
        nextIntensity -= T(2) * maxIntensity;
    }
    return Heat1DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeWave1D(T leftIntensity, T centerIntensity, T rightIntensity, T pastIntensity,
    T waveSpeed2TimeStep2OverDx2, T maxIntensity, T timeStep)
{
    const T unclampedNext = -pastIntensity + T(2) * centerIntensity +
        waveSpeed2TimeStep2OverDx2 * (leftIntensity - T(2) * centerIntensity + rightIntensity);
    const T nextIntensity = ClampRange(unclampedNext, -maxIntensity, maxIntensity);
    return Wave1DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeWave1D5(T leftLeftIntensity, T leftIntensity, T centerIntensity,
    T rightIntensity, T rightRightIntensity, T sourceVelocity, T dtOver12Dx2, T maxIntensity, T maxVelocity, T timeStep)
{
    const T dtutt = dtOver12Dx2 *
        (-leftLeftIntensity + T(16) * leftIntensity - T(30) * centerIntensity + T(16) * rightIntensity -
            rightRightIntensity);
    T nextVelocity = sourceVelocity + dtutt;
    nextVelocity = ClampRange(nextVelocity, -maxVelocity, maxVelocity);
    T nextIntensity = centerIntensity + timeStep * nextVelocity + (timeStep / T(2)) * dtutt;
    nextIntensity = WrapRange(nextIntensity, -maxIntensity, maxIntensity);
    return Wave1DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeOscillator1D(T centerIntensity, T sourceVelocity, T dtOverMass,
    T frictionMultiplier, T springMultiplier, T driverValue, T maxIntensity, T maxVelocity, T timeStep)
{
    const T nextVelocity = sourceVelocity +
        dtOverMass * (-frictionMultiplier * sourceVelocity - springMultiplier * centerIntensity + driverValue);
    const T clampedVelocity = ClampRange(nextVelocity, -maxVelocity, maxVelocity);
    T nextIntensity = centerIntensity + timeStep * nextVelocity;
    nextIntensity = ClampRange(nextIntensity, -maxIntensity, maxIntensity);
    return Wave1DResult<T>{nextIntensity, clampedVelocity};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeDiverseOscillator1D(T centerIntensity, T sourceVelocity, T dtOverMass,
    T frictionMultiplier, T springMultiplier, T driverValue, T frictionTweak, T springTweak, T massTweak,
    T maxIntensity, T maxVelocity, T timeStep)
{
    const T nextVelocity = sourceVelocity +
        dtOverMass / massTweak *
            (-frictionMultiplier * frictionTweak * sourceVelocity - springMultiplier * springTweak * centerIntensity +
                driverValue);
    const T clampedVelocity = ClampRange(nextVelocity, -maxVelocity, maxVelocity);
    T nextIntensity = centerIntensity + timeStep * nextVelocity;
    nextIntensity = ClampRange(nextIntensity, -maxIntensity, maxIntensity);
    return Wave1DResult<T>{nextIntensity, clampedVelocity};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeOscillatorWave1D(T leftIntensity, T centerIntensity, T rightIntensity,
    T sourceVelocity, T dtOverMass, T frictionMultiplier, T springMultiplier, T driverValue, T dtOverDx2,
    T maxIntensity, T maxVelocity, T timeStep)
{
    const T oscillator =
        dtOverMass * (-frictionMultiplier * sourceVelocity - springMultiplier * centerIntensity + driverValue);
    const T wave = dtOverDx2 * (leftIntensity - T(2) * centerIntensity + rightIntensity);
    const T nextVelocity = sourceVelocity + oscillator + wave;
    T nextIntensity = centerIntensity + timeStep * nextVelocity;
    nextIntensity = ClampRange(nextIntensity, -maxIntensity, maxIntensity);
    const T clampedVelocity = ClampRange(nextVelocity, -maxVelocity, maxVelocity);
    return Wave1DResult<T>{nextIntensity, clampedVelocity};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeDiverseOscillatorWave1D(T leftIntensity, T centerIntensity, T rightIntensity,
    T sourceVelocity, T dtOverMass, T frictionMultiplier, T springMultiplier, T driverValue, T frictionTweak,
    T springTweak, T massTweak, T dtOverDx2, T maxIntensity, T maxVelocity, T timeStep)
{
    const T oscillator = dtOverMass / massTweak *
        (-frictionMultiplier * frictionTweak * sourceVelocity - springMultiplier * springTweak * centerIntensity +
            driverValue);
    const T wave = dtOverDx2 * (leftIntensity - T(2) * centerIntensity + rightIntensity);
    const T nextVelocity = sourceVelocity + oscillator + wave;
    T nextIntensity = centerIntensity + timeStep * nextVelocity;
    nextIntensity = ClampRange(nextIntensity, -maxIntensity, maxIntensity);
    const T clampedVelocity = ClampRange(nextVelocity, -maxVelocity, maxVelocity);
    return Wave1DResult<T>{nextIntensity, clampedVelocity};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeUlamWave1D(T leftIntensity, T centerIntensity, T rightIntensity,
    T pastIntensity, T waveSpeed2TimeStep2OverDx2, T nonlinearity, T maxIntensity, T timeStep)
{
    const T cldiff = centerIntensity - leftIntensity;
    const T rcdiff = rightIntensity - centerIntensity;
    const T unclampedNext = -pastIntensity + T(2) * centerIntensity +
        waveSpeed2TimeStep2OverDx2 * (rcdiff - cldiff + nonlinearity * (rcdiff * rcdiff - cldiff * cldiff));
    const T nextIntensity = ClampRange(unclampedNext, -maxIntensity, maxIntensity);
    return Wave1DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
}

template <typename T>
ALPAKA_FN_HOST_ACC StableUlam1DResult<T> ComputeStableUlamWave1D(T leftIntensity, T centerIntensity, T rightIntensity,
    T sourceVelocity, T sourceTweak, T dtOverDx2, T timeStep, T nonlinearity, T maxIntensity, T maxVelocity)
{
    const T cldiff = centerIntensity - leftIntensity;
    const T rcdiff = rightIntensity - centerIntensity;
    const T dtutt = dtOverDx2 * (rcdiff - cldiff + sourceTweak * nonlinearity * (rcdiff * rcdiff - cldiff * cldiff));
    const T unclampedVelocity = sourceVelocity + dtutt;
    T nextVelocity = ClampRange(unclampedVelocity, -maxVelocity, maxVelocity);
    bool zeroNeighbors = nextVelocity != unclampedVelocity;
    const T unclampedIntensity = centerIntensity + timeStep * nextVelocity + (timeStep / T(2)) * dtutt;
    const T nextIntensity = ClampRange(unclampedIntensity, -maxIntensity, maxIntensity);
    T nextTweak = sourceTweak;
    if (nextIntensity != unclampedIntensity)
    {
        nextVelocity = T(0);
        nextTweak = T(0);
        zeroNeighbors = true;
    }
    else
    {
        nextTweak = ClampRange(sourceTweak * T(1.01), T(0.001), T(1));
    }
    return StableUlam1DResult<T>{nextIntensity, nextVelocity, nextTweak, zeroNeighbors};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeCubicUlamWave1D(T leftIntensity, T centerIntensity, T rightIntensity,
    T pastIntensity, T waveSpeed2TimeStep2OverDx2, T nonlinearity, T maxIntensity, T timeStep)
{
    const T cldiff = centerIntensity - leftIntensity;
    const T rcdiff = rightIntensity - centerIntensity;
    const T sum = rcdiff + cldiff;
    const T unclampedNext = -pastIntensity + T(2) * centerIntensity +
        waveSpeed2TimeStep2OverDx2 * ((T(1) + nonlinearity * sum * sum) * (rcdiff - cldiff));
    const T nextIntensity = ClampRange(unclampedNext, -maxIntensity, maxIntensity);
    return Wave1DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
}

template <typename T>
ALPAKA_FN_HOST_ACC Heat2DResult<T> ComputeHeat2D(T centerIntensity, T eastIntensity, T northIntensity, T westIntensity,
    T southIntensity, T heatIncrement, T maxIntensity, T timeStep)
{
    const T nabeAverage = (centerIntensity + eastIntensity + northIntensity + westIntensity + southIntensity) / T(5);
    const T nextIntensity = WrapRange(nabeAverage + heatIncrement, -maxIntensity, maxIntensity);
    return Heat2DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave2DResult<T> ComputeWave2D(T centerIntensity, T eastIntensity, T northIntensity, T westIntensity,
    T southIntensity, T pastIntensity, T waveSpeed2TimeStep2OverDx2, T maxIntensity, T timeStep)
{
    const T nabeAverage = (eastIntensity + northIntensity + westIntensity + southIntensity) / T(4);
    const T unclampedNext =
        -pastIntensity + T(2) * centerIntensity + waveSpeed2TimeStep2OverDx2 * (nabeAverage - centerIntensity);
    const T nextIntensity = ClampRange(unclampedNext, -maxIntensity, maxIntensity);
    return Wave2DResult<T>{nextIntensity, (nextIntensity - centerIntensity) / timeStep};
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

ALPAKA_FN_HOST_ACC inline std::uint32_t ComputeStandardDigitalNabeWrap(
    const std::uint8_t *source, std::uint32_t x, std::uint32_t width, std::uint32_t radius, std::uint32_t stateBits)
{
    std::uint32_t nabe = 0U;
    const std::uint32_t nabeSize = 1U + 2U * radius;
    for (std::uint32_t offset = 0U; offset < nabeSize; ++offset)
    {
        const std::uint32_t sourceX = (x + width + offset - radius) % width;
        nabe = (nabe << stateBits) | source[sourceX];
    }
    return nabe;
}

ALPAKA_FN_HOST_ACC inline std::uint8_t ComputeStandardDigitalCellWrap(const std::uint8_t *source,
    const std::uint8_t *lookup, std::uint32_t x, std::uint32_t width, std::uint32_t radius, std::uint32_t stateBits)
{
    return lookup[ComputeStandardDigitalNabeWrap(source, x, width, radius, stateBits)];
}

ALPAKA_FN_HOST_ACC inline std::uint8_t ComputeReversibleDigitalCellWrap(const std::uint8_t *source,
    const std::uint8_t *past, const std::uint8_t *lookup, std::uint32_t x, std::uint32_t width, std::uint32_t radius,
    std::uint32_t stateBits, std::uint32_t stateCount)
{
    const std::uint32_t nabe = ComputeStandardDigitalNabeWrap(source, x, width, radius, stateBits);
    return static_cast<std::uint8_t>((lookup[nabe] + stateCount - past[x]) & (stateCount - 1U));
}

template <typename T>
ALPAKA_FN_HOST_ACC Heat1DResult<T> ComputeHeat1DCell(const T *source, std::uint32_t x, std::uint32_t width, T dtOverDx2,
    T heatIncrement, T maxIntensity, T maxVelocity, T timeStep)
{
    const std::uint32_t leftX = Heat2DWrapPrevious(x, width);
    const std::uint32_t rightX = Heat2DWrapNext(x, width);
    return ComputeHeat1D<T>(
        source[leftX], source[x], source[rightX], dtOverDx2, heatIncrement, maxIntensity, maxVelocity, timeStep);
}

template <typename T>
ALPAKA_FN_HOST_ACC Heat1DResult<T> ComputeHeat1D5Cell(
    const T *source, std::uint32_t x, std::uint32_t width, T heatIncrement, T maxIntensity, T timeStep)
{
    const std::uint32_t leftX = Heat2DWrapPrevious(x, width);
    const std::uint32_t rightX = Heat2DWrapNext(x, width);
    const std::uint32_t leftLeftX = Heat2DWrapPrevious(leftX, width);
    const std::uint32_t rightRightX = Heat2DWrapNext(rightX, width);
    return ComputeHeat1D5<T>(source[leftLeftX], source[leftX], source[x], source[rightX], source[rightRightX],
        heatIncrement, maxIntensity, timeStep);
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeWave1DCell(const T *source, const T *past, std::uint32_t x,
    std::uint32_t width, T waveSpeed2TimeStep2OverDx2, T maxIntensity, T timeStep)
{
    const std::uint32_t leftX = Heat2DWrapPrevious(x, width);
    const std::uint32_t rightX = Heat2DWrapNext(x, width);
    return ComputeWave1D<T>(
        source[leftX], source[x], source[rightX], past[x], waveSpeed2TimeStep2OverDx2, maxIntensity, timeStep);
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave1DResult<T> ComputeWave1D5Cell(const T *source, const T *past, std::uint32_t x,
    std::uint32_t width, T dtOver12Dx2, T maxIntensity, T maxVelocity, T timeStep)
{
    const std::uint32_t leftX = Heat2DWrapPrevious(x, width);
    const std::uint32_t rightX = Heat2DWrapNext(x, width);
    const std::uint32_t leftLeftX = Heat2DWrapPrevious(leftX, width);
    const std::uint32_t rightRightX = Heat2DWrapNext(rightX, width);
    return ComputeWave1D5<T>(source[leftLeftX], source[leftX], source[x], source[rightX], source[rightRightX],
        (source[x] - past[x]) / timeStep, dtOver12Dx2, maxIntensity, maxVelocity, timeStep);
}

template <typename T>
ALPAKA_FN_HOST_ACC Wave2DResult<T> ComputeWave2DCell(const T *source, const T *past, std::uint32_t x, std::uint32_t y,
    std::uint32_t width, std::uint32_t height, T waveSpeed2TimeStep2OverDx2, T maxIntensity, T timeStep)
{
    const std::uint32_t center = Heat2DIndex(x, y, width);
    const std::uint32_t westX = Heat2DWrapPrevious(x, width);
    const std::uint32_t eastX = Heat2DWrapNext(x, width);
    const std::uint32_t northY = Heat2DWrapPrevious(y, height);
    const std::uint32_t southY = Heat2DWrapNext(y, height);

    return ComputeWave2D<T>(source[center], source[Heat2DIndex(eastX, y, width)], source[Heat2DIndex(x, northY, width)],
        source[Heat2DIndex(westX, y, width)], source[Heat2DIndex(x, southY, width)], past[center],
        waveSpeed2TimeStep2OverDx2, maxIntensity, timeStep);
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
