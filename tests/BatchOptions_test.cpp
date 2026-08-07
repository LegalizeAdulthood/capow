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
    EXPECT_EQ(capow::BATCH_WRAP_WRAP, result.options.wrapMode);
    EXPECT_EQ(100, result.options.steps);
    EXPECT_EQ(1946UL, result.options.seed);
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

TEST(batchOptions, parseValidSeed)
{
    const char *argv[] = {"--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--seed", "12345",
        "--output", "seed.bmp"};

    const capow::BatchParseResult result = Parse(11, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(12345UL, result.options.seed);
}

TEST(batchOptions, parseValidWrapMode)
{
    const char *argv[] = {"--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--wrap", "free",
        "--output", "free.bmp"};

    const capow::BatchParseResult result = Parse(11, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_WRAP_FREE, result.options.wrapMode);
}

TEST(batchOptions, parseValidSyntheticHeatBatch)
{
    const char *argv[] = {
        "--batch", "--backend", "gpu", "--rule", "SYNTHETIC_HEAT_2D", "--steps", "40", "--output", "heat.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_BACKEND_GPU, result.options.backend);
    EXPECT_EQ(capow::BATCH_RULE_SYNTHETIC_HEAT_2D, result.options.rule);
    EXPECT_EQ(40, result.options.steps);
    EXPECT_STREQ("SYNTHETIC_HEAT_2D", capow::BatchRuleName(result.options.rule));
}

TEST(batchOptions, parseValidHeatwaveBatch)
{
    const char *argv[] = {
        "--batch", "--backend", "gpu", "--rule", "CA_HEATWAVE", "--steps", "40", "--output", "heatwave.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_BACKEND_GPU, result.options.backend);
    EXPECT_EQ(capow::BATCH_RULE_CA_HEATWAVE, result.options.rule);
    EXPECT_EQ(40, result.options.steps);
    EXPECT_STREQ("CA_HEATWAVE", capow::BatchRuleName(result.options.rule));
}

TEST(batchOptions, parseValidHeatwave2Batch)
{
    const char *argv[] = {
        "--batch", "--backend", "gpu", "--rule", "CA_HEATWAVE2", "--steps", "40", "--output", "heatwave2.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_BACKEND_GPU, result.options.backend);
    EXPECT_EQ(capow::BATCH_RULE_CA_HEATWAVE2, result.options.rule);
    EXPECT_EQ(40, result.options.steps);
    EXPECT_STREQ("CA_HEATWAVE2", capow::BatchRuleName(result.options.rule));
}

TEST(batchOptions, parseValidWaveBatch)
{
    const char *argv[] = {"--batch", "--backend", "gpu", "--rule", "CA_WAVE", "--steps", "40", "--output", "wave.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_BACKEND_GPU, result.options.backend);
    EXPECT_EQ(capow::BATCH_RULE_CA_WAVE, result.options.rule);
    EXPECT_EQ(40, result.options.steps);
    EXPECT_STREQ("CA_WAVE", capow::BatchRuleName(result.options.rule));
}

TEST(batchOptions, parseValidWaveAliasBatch)
{
    const char *argv[] = {
        "--batch", "--backend", "gpu", "--rule", "ALT_CA_WAVE", "--steps", "40", "--output", "wave.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_RULE_CA_WAVE, result.options.rule);
    EXPECT_STREQ("CA_WAVE", capow::BatchRuleName(result.options.rule));
}

TEST(batchOptions, parseValidWave2Batch)
{
    const char *argv[] = {
        "--batch", "--backend", "gpu", "--rule", "CA_WAVE2", "--steps", "40", "--output", "wave2.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_BACKEND_GPU, result.options.backend);
    EXPECT_EQ(capow::BATCH_RULE_CA_WAVE2, result.options.rule);
    EXPECT_EQ(40, result.options.steps);
    EXPECT_STREQ("CA_WAVE2", capow::BatchRuleName(result.options.rule));
}

TEST(batchOptions, parseValidOscillatorBatch)
{
    const char *argv[] = {
        "--batch", "--backend", "gpu", "--rule", "ALT_CA_OSCILLATOR", "--steps", "40", "--output", "osc.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_RULE_CA_OSCILLATOR, result.options.rule);
    EXPECT_STREQ("CA_OSCILLATOR", capow::BatchRuleName(result.options.rule));
}

TEST(batchOptions, parseValidDiverseOscillatorBatch)
{
    const char *argv[] = {"--batch", "--backend", "gpu", "--rule", "ALT_CA_DIVERSE_OSCILLATOR", "--steps", "40",
        "--output", "diverse-osc.bmp"};

    const capow::BatchParseResult result = Parse(9, argv);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(capow::BATCH_RULE_CA_DIVERSE_OSCILLATOR, result.options.rule);
    EXPECT_STREQ("CA_DIVERSE_OSCILLATOR", capow::BatchRuleName(result.options.rule));
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

TEST(batchOptions, parseRejectsInvalidSeed)
{
    const char *argv[] = {
        "--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--seed", "0", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(11, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("invalid seed", result.error);
}

TEST(batchOptions, parseRejectsSignedSeed)
{
    const char *argv[] = {
        "--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--seed", "-1", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(11, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("invalid seed", result.error);
}

TEST(batchOptions, parseRejectsInvalidWrapMode)
{
    const char *argv[] = {"--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--wrap", "mirror",
        "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(11, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("invalid wrap mode", result.error);
}

TEST(batchOptions, parseRejectsDuplicateWrapMode)
{
    const char *argv[] = {"--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--wrap", "wrap",
        "--wrap", "free", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(13, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("duplicate --wrap", result.error);
}

TEST(batchOptions, parseRejectsDuplicateSeed)
{
    const char *argv[] = {"--batch", "--backend", "cpu", "--rule", "CA_HEAT_2D", "--steps", "100", "--seed", "1",
        "--seed", "2", "--output", "cpu.bmp"};

    const capow::BatchParseResult result = Parse(13, argv);

    EXPECT_FALSE(result.ok);
    EXPECT_EQ("duplicate --seed", result.error);
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
