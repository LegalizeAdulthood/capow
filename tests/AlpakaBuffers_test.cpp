#include "AlpakaBuffers.hpp"

#include "AlpakaBackend.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace
{

const int planeVariableCount = 2;

struct TestWavecell2
{
    capow::AlpakaPlaneValue variable[planeVariableCount];
};

void SkipWithoutGpu()
{
    if (!capow::GetAlpakaManager().IsGpuAvailable())
    {
        GTEST_SKIP() << "CUDA device unavailable";
    }
}

std::vector<TestWavecell2> MakePlane(int width, int height, capow::AlpakaPlaneValue base)
{
    std::vector<TestWavecell2> plane(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    for (std::size_t index = 0; index < plane.size(); ++index)
    {
        plane[index].variable[0] = base + static_cast<capow::AlpakaPlaneValue>(index);
        plane[index].variable[1] = capow::AlpakaPlaneValue(-1000) - static_cast<capow::AlpakaPlaneValue>(index);
    }
    return plane;
}

} // namespace

TEST(alpakaBuffers, tracksDimensionsAndDirtyState)
{
    SkipWithoutGpu();

    capow::AlpakaPlaneMirror2D mirror;

    EXPECT_FALSE(mirror.IsInitialized());
    EXPECT_FALSE(mirror.IsDirty());
    EXPECT_EQ(0, mirror.GetWidth());
    EXPECT_EQ(0, mirror.GetHeight());
    EXPECT_EQ(0U, mirror.GetCellCount());

    mirror.Resize(4, 3);

    EXPECT_TRUE(mirror.IsInitialized());
    EXPECT_TRUE(mirror.IsDirty());
    EXPECT_EQ(4, mirror.GetWidth());
    EXPECT_EQ(3, mirror.GetHeight());
    EXPECT_EQ(12U, mirror.GetCellCount());
}

TEST(alpakaBuffers, sourcePlaneRoundTripsThroughTarget)
{
    SkipWithoutGpu();

    const int width = 5;
    const int height = 4;
    std::vector<TestWavecell2> source = MakePlane(width, height, capow::AlpakaPlaneValue(10));
    std::vector<TestWavecell2> past = MakePlane(width, height, capow::AlpakaPlaneValue(200));
    std::vector<TestWavecell2> target = MakePlane(width, height, capow::AlpakaPlaneValue(-500));

    capow::AlpakaPlaneMirror2D mirror;
    mirror.Resize(width, height);
    mirror.CopySourceAndPastToDevice(&source[0].variable[0], &past[0].variable[0], planeVariableCount);
    mirror.DebugCopySourceToTarget();
    mirror.CopyTargetToHost(&target[0].variable[0], planeVariableCount);

    EXPECT_FALSE(mirror.IsDirty());
    for (std::size_t index = 0; index < source.size(); ++index)
    {
        EXPECT_EQ(source[index].variable[0], target[index].variable[0]);
        EXPECT_EQ(
            capow::AlpakaPlaneValue(-1000) - static_cast<capow::AlpakaPlaneValue>(index), target[index].variable[1]);
    }
}

TEST(alpakaBuffers, pastPlaneRoundTripsThroughTarget)
{
    SkipWithoutGpu();

    const int width = 3;
    const int height = 3;
    std::vector<TestWavecell2> source = MakePlane(width, height, capow::AlpakaPlaneValue(1));
    std::vector<TestWavecell2> past = MakePlane(width, height, capow::AlpakaPlaneValue(50));
    std::vector<TestWavecell2> target = MakePlane(width, height, capow::AlpakaPlaneValue(-50));

    capow::AlpakaPlaneMirror2D mirror;
    mirror.Resize(width, height);
    mirror.CopySourceAndPastToDevice(&source[0].variable[0], &past[0].variable[0], planeVariableCount);
    mirror.DebugCopyPastToTarget();
    mirror.CopyTargetToHost(&target[0].variable[0], planeVariableCount);

    EXPECT_FALSE(mirror.IsDirty());
    for (std::size_t index = 0; index < past.size(); ++index)
    {
        EXPECT_EQ(past[index].variable[0], target[index].variable[0]);
        EXPECT_EQ(
            capow::AlpakaPlaneValue(-1000) - static_cast<capow::AlpakaPlaneValue>(index), target[index].variable[1]);
    }
}
