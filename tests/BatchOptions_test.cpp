#include "BatchOptions.hpp"

#include <gtest/gtest.h>

namespace
{

capow::BatchParseResult Parse(int argc, const char *argv[])
{
    return capow::ParseBatchArguments(argc, argv);
}

} // namespace

TEST(batchOptions, parseIgnoresNonBatchArguments)
{
    const char *argv[] = {"startup.CA"};

    const capow::BatchParseResult result = Parse(1, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(result.batch);
}

TEST(batchOptions, parseValidCpuBatch)
{
    const char *argv[] = {
        "--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.batch);
    EXPECT_TRUE(result.options.batch);
    EXPECT_EQ(capow::BATCH_BACKEND_CPU, result.options.backend);
    EXPECT_EQ(capow::BATCH_RULE_CA_HEAT_2D, result.options.rule);
    EXPECT_EQ(100, result.options.steps);
    EXPECT_EQ("cpu.bmp", result.options.output);
}

TEST(batchOptions, parseValidGpuBatch)
{
    const char *argv[] = {
        "--batch", "--backend", "gpu", "--rule", "CA_WAVE_2D", "--steps", "25", "--output", "gpu.BMP"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_BACKEND_GPU, result.options.backend);
    EXPECT_EQ(capow::BATCH_RULE_CA_WAVE_2D, result.options.rule);
    EXPECT_EQ(25, result.options.steps);
}

TEST(batchOptions, parseRejectsMissingBackend)
{
    const char *argv[] = {"--batch", "--rule", "CA_HEAT_2D", "--steps", "100", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(7, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(result.batch);
    EXPECT_EQ("missing --backend", result.error);
}

TEST(batchOptions, parseRejectsMissingOutput)
{
    const char *argv[] = {"--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100"};

    const capow::BatchParseResult result = Parse(7, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("missing --output", result.error);
}

TEST(batchOptions, parseRejectsInvalidSteps)
{
    const char *argv[] = {
        "--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "zero", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("invalid step count", result.error);
}

TEST(batchOptions, parseRejectsUnknownRule)
{
    const char *argv[] = {
        "--batch", "--backend", "cpu", "--rule", "CA_UNKNOWN", "--steps", "100", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("invalid rule", result.error);
}

TEST(batchOptions, parseTokenizesQuotedOutputPath)
{
    const capow::BatchParseResult result =
        capow::ParseBatchCommandLine("--batch --backend cpu --rule CA_HEAT_2D --steps 100 "
                                     "--output \"C:\\tmp\\cpu image.bmp\"");

    EXPECT_TRUE(result.ok);
    EXPECT_EQ("C:\\tmp\\cpu image.bmp", result.options.output);
}
