#pragma once

#include <cstdint>

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
namespace alpaka_util
{
struct FiveNeighborIndexes
{
    std::uint32_t center;
    std::uint32_t north;
    std::uint32_t south;
    std::uint32_t west;
    std::uint32_t east;
};

struct NineNeighborIndexes
{
    std::uint32_t center;
    std::uint32_t north;
    std::uint32_t south;
    std::uint32_t west;
    std::uint32_t east;
    std::uint32_t northwest;
    std::uint32_t northeast;
    std::uint32_t southwest;
    std::uint32_t southeast;
};

template <typename T>
ALPAKA_FN_HOST_ACC inline T clamp(T value, T minimum, T maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

ALPAKA_FN_HOST_ACC inline std::uint32_t wrap_prev(std::uint32_t value, std::uint32_t extent)
{
    return value == 0U ? extent - 1U : value - 1U;
}

ALPAKA_FN_HOST_ACC inline std::uint32_t wrap_next(std::uint32_t value, std::uint32_t extent)
{
    return value + 1U == extent ? 0U : value + 1U;
}

ALPAKA_FN_HOST_ACC inline std::uint32_t index_2d(std::uint32_t x, std::uint32_t y, std::uint32_t width)
{
    return y * width + x;
}

ALPAKA_FN_HOST_ACC inline FiveNeighborIndexes five_neighbor_indexes(
    std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height)
{
    std::uint32_t const north_y = wrap_prev(y, height);
    std::uint32_t const south_y = wrap_next(y, height);
    std::uint32_t const west_x = wrap_prev(x, width);
    std::uint32_t const east_x = wrap_next(x, width);

    return FiveNeighborIndexes{index_2d(x, y, width), index_2d(x, north_y, width), index_2d(x, south_y, width),
        index_2d(west_x, y, width), index_2d(east_x, y, width)};
}

ALPAKA_FN_HOST_ACC inline NineNeighborIndexes nine_neighbor_indexes(
    std::uint32_t x, std::uint32_t y, std::uint32_t width, std::uint32_t height)
{
    std::uint32_t const north_y = wrap_prev(y, height);
    std::uint32_t const south_y = wrap_next(y, height);
    std::uint32_t const west_x = wrap_prev(x, width);
    std::uint32_t const east_x = wrap_next(x, width);

    return NineNeighborIndexes{index_2d(x, y, width), index_2d(x, north_y, width), index_2d(x, south_y, width),
        index_2d(west_x, y, width), index_2d(east_x, y, width), index_2d(west_x, north_y, width),
        index_2d(east_x, north_y, width), index_2d(west_x, south_y, width), index_2d(east_x, south_y, width)};
}
} // namespace alpaka_util
} // namespace capow
