#include "CapowRules.hpp"

#include <gtest/gtest.h>

TEST(capowRules, heat1dUsesImplicitThreePointAverage)
{
    const capow::Heat1DResult<float> result =
        capow::ComputeHeat1D(10.0F, 20.0F, -5.0F, 0.25F, 2.0F, 100.0F, 10.0F, 0.5F);

    EXPECT_FLOAT_EQ(15.166667F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-9.666667F, result.velocity);
}

TEST(capowRules, heat1dClampsVelocityBeforeIntensityWrap)
{
    const capow::Heat1DResult<float> result = capow::ComputeHeat1D(8.0F, 8.0F, 8.0F, 0.5F, 12.0F, 10.0F, 4.0F, 0.5F);

    EXPECT_FLOAT_EQ(-6.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(4.0F, result.velocity);
}

TEST(capowRules, heat1dCellUsesWrapBoundary)
{
    const float field[] = {0.0F, 10.0F, 20.0F};

    const capow::Heat1DResult<float> result = capow::ComputeHeat1DCell(field, 0U, 3U, 0.5F, 0.0F, 100.0F, 10.0F, 1.0F);

    EXPECT_FLOAT_EQ(7.5F, result.nextIntensity);
    EXPECT_FLOAT_EQ(7.5F, result.velocity);
}

TEST(capowRules, heat1dFiveNeighborAveragesInputs)
{
    const capow::Heat1DResult<float> result = capow::ComputeHeat1D5(1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 0.5F, 10.0F, 0.25F);

    EXPECT_FLOAT_EQ(3.5F, result.nextIntensity);
    EXPECT_FLOAT_EQ(2.0F, result.velocity);
}

TEST(capowRules, heat1dFiveNeighborWrapsTopOnly)
{
    const capow::Heat1DResult<float> result = capow::ComputeHeat1D5(9.0F, 9.0F, 9.0F, 9.0F, 9.0F, 3.0F, 10.0F, 0.5F);

    EXPECT_FLOAT_EQ(-8.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-34.0F, result.velocity);
}

TEST(capowRules, heat1dFiveNeighborCellUsesWrapBoundary)
{
    const float field[] = {0.0F, 10.0F, 20.0F, 30.0F, 40.0F};

    const capow::Heat1DResult<float> result = capow::ComputeHeat1D5Cell(field, 0U, 5U, 0.0F, 100.0F, 1.0F);

    EXPECT_FLOAT_EQ(20.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(20.0F, result.velocity);
}

TEST(capowRules, wave1dUsesThreeNeighborSecondDifference)
{
    const capow::Wave1DResult<float> result = capow::ComputeWave1D(2.0F, 3.0F, 4.0F, 1.0F, 0.5F, 100.0F, 0.25F);

    EXPECT_FLOAT_EQ(5.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(8.0F, result.velocity);
}

TEST(capowRules, wave1dClampsAboveMaxIntensity)
{
    const capow::Wave1DResult<float> result = capow::ComputeWave1D(0.0F, 100.0F, 0.0F, -100.0F, 0.0F, 10.0F, 2.0F);

    EXPECT_FLOAT_EQ(10.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-45.0F, result.velocity);
}

TEST(capowRules, wave1dClampsBelowMaxIntensity)
{
    const capow::Wave1DResult<float> result = capow::ComputeWave1D(0.0F, -100.0F, 0.0F, 100.0F, 0.0F, 10.0F, 2.0F);

    EXPECT_FLOAT_EQ(-10.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(45.0F, result.velocity);
}

TEST(capowRules, wave1dCellUsesWrapBoundary)
{
    const float source[] = {0.0F, 10.0F, 20.0F};
    const float past[] = {5.0F, 6.0F, 7.0F};

    const capow::Wave1DResult<float> result = capow::ComputeWave1DCell(source, past, 0U, 3U, 0.5F, 100.0F, 1.0F);

    EXPECT_FLOAT_EQ(10.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(10.0F, result.velocity);
}

TEST(capowRules, wave1dFiveNeighborUsesFourthOrderSecondDifference)
{
    const capow::Wave1DResult<float> result =
        capow::ComputeWave1D5(1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 0.5F, 0.25F, 100.0F, 10.0F, 0.5F);

    EXPECT_FLOAT_EQ(3.25F, result.nextIntensity);
    EXPECT_FLOAT_EQ(0.5F, result.velocity);
}

TEST(capowRules, wave1dFiveNeighborClampsVelocityBeforeIntensityWrap)
{
    const capow::Wave1DResult<float> result =
        capow::ComputeWave1D5(0.0F, 0.0F, 10.0F, 0.0F, 0.0F, 20.0F, 0.1F, 10.0F, 1.0F, 1.0F);

    EXPECT_FLOAT_EQ(-6.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-16.0F, result.velocity);
}

TEST(capowRules, wave1dFiveNeighborCellUsesWrapBoundary)
{
    const float source[] = {0.0F, 10.0F, 20.0F, 30.0F, 40.0F};
    const float past[] = {-1.0F, 9.0F, 19.0F, 29.0F, 39.0F};

    const capow::Wave1DResult<float> result =
        capow::ComputeWave1D5Cell(source, past, 0U, 5U, 0.01F, 100.0F, 10.0F, 1.0F);

    EXPECT_FLOAT_EQ(12.25F, result.nextIntensity);
    EXPECT_FLOAT_EQ(12.25F, result.velocity);
}

TEST(capowRules, heat2dAveragesFiveInputs)
{
    const capow::Heat2DResult<float> result = capow::ComputeHeat2D(10.0F, 20.0F, -5.0F, 5.0F, 0.0F, 2.0F, 100.0F, 0.5F);

    EXPECT_FLOAT_EQ(8.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-4.0F, result.velocity);
}

TEST(capowRules, heat2dWrapsAboveMaxIntensity)
{
    const capow::Heat2DResult<float> result = capow::ComputeHeat2D(4.0F, 4.0F, 4.0F, 4.0F, 4.0F, 8.0F, 10.0F, 2.0F);

    EXPECT_FLOAT_EQ(-8.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-6.0F, result.velocity);
}

TEST(capowRules, heat2dWrapsBelowMaxIntensity)
{
    const capow::Heat2DResult<float> result =
        capow::ComputeHeat2D(-4.0F, -4.0F, -4.0F, -4.0F, -4.0F, -8.0F, 10.0F, 2.0F);

    EXPECT_FLOAT_EQ(8.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(6.0F, result.velocity);
}

TEST(capowRules, heat2dMatchesFarWrapBehavior)
{
    const capow::Heat2DResult<float> result = capow::ComputeHeat2D(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 25.0F, 10.0F, 1.0F);

    EXPECT_FLOAT_EQ(-10.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-10.0F, result.velocity);
}

TEST(capowRules, heat2dCellUsesWrapBoundary)
{
    const float field[] = {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};

    const capow::Heat2DResult<float> result =
        capow::ComputeHeat2DCell(field, 0U, 0U, 3U, 3U, capow::HEAT_2D_BOUNDARY_WRAP, 0.0F, 100.0F, 1.0F);

    EXPECT_FLOAT_EQ(2.4F, result.nextIntensity);
    EXPECT_FLOAT_EQ(2.4F, result.velocity);
}

TEST(capowRules, heat2dCellUsesFreeBoundary)
{
    const float field[] = {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};

    const capow::Heat2DResult<float> result =
        capow::ComputeHeat2DCell(field, 0U, 0U, 3U, 3U, capow::HEAT_2D_BOUNDARY_FREE, 0.0F, 100.0F, 1.0F);

    EXPECT_FLOAT_EQ(0.8F, result.nextIntensity);
    EXPECT_FLOAT_EQ(0.8F, result.velocity);
}

TEST(capowRules, heat2dCellUsesAbsorbBoundary)
{
    const float field[] = {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};

    const capow::Heat2DResult<float> result =
        capow::ComputeHeat2DCell(field, 0U, 0U, 3U, 3U, capow::HEAT_2D_BOUNDARY_ABSORB, 0.0F, 100.0F, 1.0F);

    EXPECT_FLOAT_EQ(4.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(0.0F, result.velocity);
}

TEST(capowRules, heat2dCellUsesZeroBoundary)
{
    const float field[] = {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};

    const capow::Heat2DResult<float> result =
        capow::ComputeHeat2DCell(field, 2U, 1U, 3U, 3U, capow::HEAT_2D_BOUNDARY_ZERO, 0.0F, 100.0F, 1.0F);

    EXPECT_FLOAT_EQ(0.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(0.0F, result.velocity);
}

TEST(capowRules, heat2dCellUsesFixedBoundary)
{
    const float field[] = {0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F};

    const capow::Heat2DResult<float> result =
        capow::ComputeHeat2DCell(field, 2U, 1U, 3U, 3U, capow::HEAT_2D_BOUNDARY_FIXED, 0.0F, 100.0F, 1.0F);

    EXPECT_FLOAT_EQ(5.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(0.0F, result.velocity);
}

TEST(capowRules, wave2dUsesFourNeighborAverage)
{
    const capow::Wave2DResult<float> result =
        capow::ComputeWave2D(10.0F, 20.0F, 15.0F, 5.0F, -5.0F, 7.0F, 0.5F, 100.0F, 0.25F);

    EXPECT_FLOAT_EQ(12.375F, result.nextIntensity);
    EXPECT_FLOAT_EQ(9.5F, result.velocity);
}

TEST(capowRules, wave2dClampsAboveMaxIntensity)
{
    const capow::Wave2DResult<float> result =
        capow::ComputeWave2D(90.0F, 200.0F, 200.0F, 200.0F, 200.0F, -90.0F, 1.0F, 100.0F, 2.0F);

    EXPECT_FLOAT_EQ(100.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(5.0F, result.velocity);
}

TEST(capowRules, wave2dClampsBelowMaxIntensity)
{
    const capow::Wave2DResult<float> result =
        capow::ComputeWave2D(-90.0F, -200.0F, -200.0F, -200.0F, -200.0F, 90.0F, 1.0F, 100.0F, 2.0F);

    EXPECT_FLOAT_EQ(-100.0F, result.nextIntensity);
    EXPECT_FLOAT_EQ(-5.0F, result.velocity);
}
