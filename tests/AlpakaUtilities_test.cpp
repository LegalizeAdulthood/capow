#include "AlpakaUtilities.hpp"

#include <gtest/gtest.h>

TEST(AlpakaUtilities, clampLimitsValues)
{
    EXPECT_EQ(0, capow::alpaka_util::clamp(-5, 0, 10));
    EXPECT_EQ(7, capow::alpaka_util::clamp(7, 0, 10));
    EXPECT_EQ(10, capow::alpaka_util::clamp(17, 0, 10));
    EXPECT_FLOAT_EQ(0.25F, capow::alpaka_util::clamp(0.25F, 0.0F, 1.0F));
}

TEST(AlpakaUtilities, wrapIndexes)
{
    EXPECT_EQ(4U, capow::alpaka_util::wrap_prev(0U, 5U));
    EXPECT_EQ(2U, capow::alpaka_util::wrap_prev(3U, 5U));
    EXPECT_EQ(1U, capow::alpaka_util::wrap_next(0U, 5U));
    EXPECT_EQ(0U, capow::alpaka_util::wrap_next(4U, 5U));
}

TEST(AlpakaUtilities, index2dMapsCoordinates)
{
    EXPECT_EQ(0U, capow::alpaka_util::index_2d(0U, 0U, 4U));
    EXPECT_EQ(6U, capow::alpaka_util::index_2d(2U, 1U, 4U));
    EXPECT_EQ(11U, capow::alpaka_util::index_2d(3U, 2U, 4U));
}

TEST(AlpakaUtilities, fiveNeighborIndexesWrap)
{
    capow::alpaka_util::FiveNeighborIndexes const indexes = capow::alpaka_util::five_neighbor_indexes(0U, 0U, 4U, 3U);

    EXPECT_EQ(0U, indexes.center);
    EXPECT_EQ(8U, indexes.north);
    EXPECT_EQ(4U, indexes.south);
    EXPECT_EQ(3U, indexes.west);
    EXPECT_EQ(1U, indexes.east);
}

TEST(AlpakaUtilities, nineNeighborIndexesWrap)
{
    capow::alpaka_util::NineNeighborIndexes const indexes = capow::alpaka_util::nine_neighbor_indexes(0U, 0U, 4U, 3U);

    EXPECT_EQ(0U, indexes.center);
    EXPECT_EQ(8U, indexes.north);
    EXPECT_EQ(4U, indexes.south);
    EXPECT_EQ(3U, indexes.west);
    EXPECT_EQ(1U, indexes.east);
    EXPECT_EQ(11U, indexes.northwest);
    EXPECT_EQ(9U, indexes.northeast);
    EXPECT_EQ(7U, indexes.southwest);
    EXPECT_EQ(5U, indexes.southeast);
}
