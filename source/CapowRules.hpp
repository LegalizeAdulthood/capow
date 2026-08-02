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

namespace capow
{

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

} // namespace capow

#endif
