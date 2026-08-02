#include "CapowRules.hpp"

#include <gtest/gtest.h>

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
