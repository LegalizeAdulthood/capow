#include "AlpakaBuffers.hpp"

#include "AlpakaBackend.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace
{

const int planeVariableCount = 2;
const int rowVariableCount = 2;

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

std::vector<capow::AlpakaPlaneValue> MakeRow(
    int width, capow::AlpakaPlaneValue intensityBase, capow::AlpakaPlaneValue velocityBase)
{
    std::vector<capow::AlpakaPlaneValue> row(static_cast<std::size_t>(width) * rowVariableCount);
    for (int index = 0; index < width; ++index)
    {
        const std::size_t offset = static_cast<std::size_t>(index) * rowVariableCount;
        row[offset] = intensityBase + static_cast<capow::AlpakaPlaneValue>(index);
        row[offset + 1U] = velocityBase - static_cast<capow::AlpakaPlaneValue>(index);
    }
    return row;
}

void ExpectRowsEqual(
    const std::vector<capow::AlpakaPlaneValue> &expected, const std::vector<capow::AlpakaPlaneValue> &actual)
{
    ASSERT_EQ(expected.size(), actual.size());
    for (std::size_t index = 0; index < expected.size(); index += rowVariableCount)
    {
        EXPECT_EQ(expected[index], actual[index]);
        EXPECT_EQ(expected[index + 1U], actual[index + 1U]);
    }
}

std::vector<capow::AlpakaDigitalValue> MakeDigitalBytes(int count, unsigned int base)
{
    std::vector<capow::AlpakaDigitalValue> values(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index)
    {
        values[static_cast<std::size_t>(index)] =
            static_cast<capow::AlpakaDigitalValue>((base + static_cast<unsigned int>(index) * 17U) & 0xFFU);
    }
    return values;
}

void ExpectBytesEqual(
    const std::vector<capow::AlpakaDigitalValue> &expected, const std::vector<capow::AlpakaDigitalValue> &actual)
{
    ASSERT_EQ(expected.size(), actual.size());
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        EXPECT_EQ(expected[index], actual[index]) << "byte index " << index;
    }
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

TEST(alpakaBuffers, continuousRowTracksDimensionsDirtyStateAndSlots)
{
    SkipWithoutGpu();

    capow::AlpakaContinuousRowMirror1D mirror;

    EXPECT_FALSE(mirror.IsInitialized());
    EXPECT_FALSE(mirror.IsDirty());
    EXPECT_EQ(0, mirror.GetWidth());
    EXPECT_EQ(0U, mirror.GetCellCount());
    EXPECT_EQ(0, mirror.GetSourceSlot());
    EXPECT_EQ(1, mirror.GetTargetSlot());
    EXPECT_EQ(2, mirror.GetPastSlot());

    mirror.Resize(7);

    EXPECT_TRUE(mirror.IsInitialized());
    EXPECT_TRUE(mirror.IsDirty());
    EXPECT_EQ(7, mirror.GetWidth());
    EXPECT_EQ(7U, mirror.GetCellCount());
    EXPECT_EQ(0, mirror.GetSourceSlot());
    EXPECT_EQ(1, mirror.GetTargetSlot());
    EXPECT_EQ(2, mirror.GetPastSlot());
}

TEST(alpakaBuffers, continuousRowSourceRoundTripsThroughTarget)
{
    SkipWithoutGpu();

    const int width = 8;
    std::vector<capow::AlpakaPlaneValue> source =
        MakeRow(width, capow::AlpakaPlaneValue(10), capow::AlpakaPlaneValue(100));
    std::vector<capow::AlpakaPlaneValue> past =
        MakeRow(width, capow::AlpakaPlaneValue(200), capow::AlpakaPlaneValue(300));
    std::vector<capow::AlpakaPlaneValue> target =
        MakeRow(width, capow::AlpakaPlaneValue(-500), capow::AlpakaPlaneValue(-600));

    capow::AlpakaContinuousRowMirror1D mirror;
    mirror.Resize(width);
    mirror.CopySourceAndPastToDevice(&source[0], &source[1], &past[0], &past[1], rowVariableCount);
    mirror.DebugCopySourceToTarget();
    mirror.CopyTargetToHost(&target[0], &target[1], rowVariableCount);

    EXPECT_FALSE(mirror.IsDirty());
    ExpectRowsEqual(source, target);
}

TEST(alpakaBuffers, continuousRowPastRoundTripsThroughTarget)
{
    SkipWithoutGpu();

    const int width = 5;
    std::vector<capow::AlpakaPlaneValue> source =
        MakeRow(width, capow::AlpakaPlaneValue(1), capow::AlpakaPlaneValue(2));
    std::vector<capow::AlpakaPlaneValue> past =
        MakeRow(width, capow::AlpakaPlaneValue(50), capow::AlpakaPlaneValue(60));
    std::vector<capow::AlpakaPlaneValue> target =
        MakeRow(width, capow::AlpakaPlaneValue(-50), capow::AlpakaPlaneValue(-60));

    capow::AlpakaContinuousRowMirror1D mirror;
    mirror.Resize(width);
    mirror.CopySourceAndPastToDevice(&source[0], &source[1], &past[0], &past[1], rowVariableCount);
    mirror.DebugCopyPastToTarget();
    mirror.CopyTargetToHost(&target[0], &target[1], rowVariableCount);

    EXPECT_FALSE(mirror.IsDirty());
    ExpectRowsEqual(past, target);
}

TEST(alpakaBuffers, continuousRowTargetMapsToDisplayRow)
{
    SkipWithoutGpu();

    const int width = 6;
    std::vector<capow::AlpakaPlaneValue> source =
        MakeRow(width, capow::AlpakaPlaneValue(70), capow::AlpakaPlaneValue(700));
    std::vector<capow::AlpakaPlaneValue> past =
        MakeRow(width, capow::AlpakaPlaneValue(80), capow::AlpakaPlaneValue(800));
    std::vector<capow::AlpakaPlaneValue> display =
        MakeRow(width, capow::AlpakaPlaneValue(-70), capow::AlpakaPlaneValue(-700));

    capow::AlpakaContinuousRowMirror1D mirror;
    mirror.Resize(width);
    mirror.CopySourceAndPastToDevice(&source[0], &source[1], &past[0], &past[1], rowVariableCount);
    mirror.DebugCopySourceToTarget();
    mirror.CopyTargetToDisplayRow();
    mirror.CopyDisplayRowToHost(&display[0], &display[1], rowVariableCount);

    EXPECT_FALSE(mirror.IsDirty());
    ExpectRowsEqual(source, display);
}

TEST(alpakaBuffers, continuousRowRotationMatchesWaveUpdateStep)
{
    SkipWithoutGpu();

    capow::AlpakaContinuousRowMirror1D mirror;
    mirror.Resize(4);

    mirror.RotateRows();

    EXPECT_EQ(1, mirror.GetSourceSlot());
    EXPECT_EQ(2, mirror.GetTargetSlot());
    EXPECT_EQ(0, mirror.GetPastSlot());

    mirror.RotateRows();

    EXPECT_EQ(2, mirror.GetSourceSlot());
    EXPECT_EQ(0, mirror.GetTargetSlot());
    EXPECT_EQ(1, mirror.GetPastSlot());

    mirror.RotateRows();

    EXPECT_EQ(0, mirror.GetSourceSlot());
    EXPECT_EQ(1, mirror.GetTargetSlot());
    EXPECT_EQ(2, mirror.GetPastSlot());
}

TEST(alpakaBuffers, digitalRowTracksDimensionsDirtyStateAndSlots)
{
    SkipWithoutGpu();

    capow::AlpakaDigitalRowMirror1D mirror;

    EXPECT_FALSE(mirror.IsInitialized());
    EXPECT_FALSE(mirror.IsDirty());
    EXPECT_FALSE(mirror.HasPastRow());
    EXPECT_EQ(0, mirror.GetWidth());
    EXPECT_EQ(0, mirror.GetLookupCount());
    EXPECT_EQ(0U, mirror.GetCellCount());
    EXPECT_EQ(0, mirror.GetSourceSlot());
    EXPECT_EQ(1, mirror.GetTargetSlot());
    EXPECT_EQ(2, mirror.GetPastSlot());

    mirror.Resize(9, 32, false);

    EXPECT_TRUE(mirror.IsInitialized());
    EXPECT_TRUE(mirror.IsDirty());
    EXPECT_FALSE(mirror.HasPastRow());
    EXPECT_EQ(9, mirror.GetWidth());
    EXPECT_EQ(32, mirror.GetLookupCount());
    EXPECT_EQ(9U, mirror.GetCellCount());
    EXPECT_EQ(0, mirror.GetSourceSlot());
    EXPECT_EQ(1, mirror.GetTargetSlot());
    EXPECT_EQ(2, mirror.GetPastSlot());

    mirror.Resize(9, 32, true);

    EXPECT_TRUE(mirror.HasPastRow());
    EXPECT_EQ(0, mirror.GetSourceSlot());
    EXPECT_EQ(1, mirror.GetTargetSlot());
    EXPECT_EQ(2, mirror.GetPastSlot());
}

TEST(alpakaBuffers, digitalRowSourceRoundTripsThroughTarget)
{
    SkipWithoutGpu();

    const int width = 11;
    const int lookupCount = 64;
    std::vector<capow::AlpakaDigitalValue> source = MakeDigitalBytes(width, 3U);
    std::vector<capow::AlpakaDigitalValue> target = MakeDigitalBytes(width, 200U);
    std::vector<capow::AlpakaDigitalValue> lookup = MakeDigitalBytes(lookupCount, 19U);

    capow::AlpakaDigitalRowMirror1D mirror;
    mirror.Resize(width, lookupCount, false);
    mirror.CopyRowsAndLookupToDevice(source.data(), target.data(), nullptr, lookup.data());
    mirror.DebugCopySourceToTarget();
    mirror.CopyTargetToHost(target.data());

    EXPECT_FALSE(mirror.IsDirty());
    ExpectBytesEqual(source, target);
}

TEST(alpakaBuffers, digitalRowPastRoundTripsThroughTarget)
{
    SkipWithoutGpu();

    const int width = 7;
    const int lookupCount = 16;
    std::vector<capow::AlpakaDigitalValue> source = MakeDigitalBytes(width, 5U);
    std::vector<capow::AlpakaDigitalValue> target = MakeDigitalBytes(width, 90U);
    std::vector<capow::AlpakaDigitalValue> past = MakeDigitalBytes(width, 131U);
    std::vector<capow::AlpakaDigitalValue> lookup = MakeDigitalBytes(lookupCount, 29U);

    capow::AlpakaDigitalRowMirror1D mirror;
    mirror.Resize(width, lookupCount, true);
    mirror.CopyRowsAndLookupToDevice(source.data(), target.data(), past.data(), lookup.data());
    mirror.DebugCopyPastToTarget();
    mirror.CopyTargetToHost(target.data());

    EXPECT_FALSE(mirror.IsDirty());
    ExpectBytesEqual(past, target);
}

TEST(alpakaBuffers, digitalRowLookupRoundTripsExactly)
{
    SkipWithoutGpu();

    const int width = 5;
    const int lookupCount = 128;
    std::vector<capow::AlpakaDigitalValue> source = MakeDigitalBytes(width, 11U);
    std::vector<capow::AlpakaDigitalValue> target = MakeDigitalBytes(width, 12U);
    std::vector<capow::AlpakaDigitalValue> lookup = MakeDigitalBytes(lookupCount, 41U);
    std::vector<capow::AlpakaDigitalValue> lookupCopy(lookup.size(), capow::AlpakaDigitalValue(0));

    capow::AlpakaDigitalRowMirror1D mirror;
    mirror.Resize(width, lookupCount, false);
    mirror.CopyRowsAndLookupToDevice(source.data(), target.data(), nullptr, lookup.data());
    mirror.CopyLookupToHost(lookupCopy.data());

    EXPECT_FALSE(mirror.IsDirty());
    ExpectBytesEqual(lookup, lookupCopy);
}

TEST(alpakaBuffers, digitalRowRotationMatchesReversibleUpdateStep)
{
    SkipWithoutGpu();

    capow::AlpakaDigitalRowMirror1D mirror;
    mirror.Resize(4, 16, true);

    mirror.RotateRows();

    EXPECT_EQ(1, mirror.GetSourceSlot());
    EXPECT_EQ(2, mirror.GetTargetSlot());
    EXPECT_EQ(0, mirror.GetPastSlot());

    mirror.RotateRows();

    EXPECT_EQ(2, mirror.GetSourceSlot());
    EXPECT_EQ(0, mirror.GetTargetSlot());
    EXPECT_EQ(1, mirror.GetPastSlot());

    mirror.RotateRows();

    EXPECT_EQ(0, mirror.GetSourceSlot());
    EXPECT_EQ(1, mirror.GetTargetSlot());
    EXPECT_EQ(2, mirror.GetPastSlot());
}
