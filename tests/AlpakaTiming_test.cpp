#include "AlpakaTiming.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

TEST(alpakaTiming, accumulatesCategoriesAndTotalsGpuFields)
{
    capow::AlpakaTimingMeasurements timing;

    capow::AddAlpakaTiming(&timing, capow::ALPAKA_TIMING_CPU_UPDATE, 1.25);
    capow::AddAlpakaTiming(&timing, capow::ALPAKA_TIMING_GPU_UPDATE_ONLY, 2.0);
    capow::AddAlpakaTiming(&timing, capow::ALPAKA_TIMING_GPU_INTEROP, 3.0);
    capow::AddAlpakaTiming(&timing, capow::ALPAKA_TIMING_GPU_TEXTURE, 4.0);
    capow::AddAlpakaTiming(&timing, capow::ALPAKA_TIMING_GPU_RENDER, 5.0);
    capow::AddAlpakaTiming(&timing, capow::ALPAKA_TIMING_GPU_BATCH_SAVE_READBACK, 6.0);
    capow::AddAlpakaTiming(&timing, capow::ALPAKA_TIMING_GPU_UPDATE_ONLY, 0.5);

    EXPECT_DOUBLE_EQ(1.25, timing.cpuUpdateMs);
    EXPECT_DOUBLE_EQ(2.5, timing.gpuUpdateOnlyMs);
    EXPECT_DOUBLE_EQ(20.5, capow::AlpakaGpuTotalMs(timing));
}

TEST(alpakaTiming, ignoresNullAccumulator)
{
    capow::AddAlpakaTiming(nullptr, capow::ALPAKA_TIMING_GPU_UPDATE_ONLY, 3.0);
}

TEST(alpakaTiming, formatsMarkdownTableWithRequiredRows)
{
    std::vector<capow::AlpakaTimingRow> rows(2U);
    rows[0].rule = "CA_HEAT_2D";
    rows[0].width = 500;
    rows[0].height = 250;
    rows[0].steps = 1000;
    rows[0].timing.cpuUpdateMs = 100.0;
    rows[0].timing.gpuUpdateOnlyMs = 10.0;
    rows[0].timing.gpuBatchSaveReadbackMs = 0.5;
    rows[0].maxError = 0.125;

    rows[1].rule = "CA_WAVE_2D";
    rows[1].width = 400;
    rows[1].height = 200;
    rows[1].steps = 50;
    rows[1].timing.cpuUpdateMs = 20.0;
    rows[1].timing.gpuUpdateOnlyMs = 2.0;
    rows[1].timing.gpuRenderMs = 1.0;

    const std::string markdown = capow::FormatAlpakaTimingMarkdownTable(rows);

    EXPECT_NE(std::string::npos, markdown.find("| metric | CA_HEAT_2D | CA_WAVE_2D |"));
    EXPECT_NE(std::string::npos, markdown.find("| grid | 500x250 | 400x200 |"));
    EXPECT_NE(std::string::npos, markdown.find("| gpu_batch_save_readback_ms | 0.5 | 0 |"));
    EXPECT_NE(std::string::npos, markdown.find("| gpu_total_ms | 10.5 | 3 |"));
    EXPECT_NE(std::string::npos, markdown.find("| max_error | 0.125 | 0 |"));
}

TEST(alpakaTiming, formatsMarkdownDocumentMetadata)
{
    std::vector<capow::AlpakaTimingRow> rows(1U);
    rows[0].rule = "CA_HEAT_2D";

    const std::string markdown =
        capow::FormatAlpakaTimingMarkdownDocument("Title", "2026-08-06", "Debug", "Scope text.", rows);

    EXPECT_NE(std::string::npos, markdown.find("# Title\n\n"));
    EXPECT_NE(std::string::npos, markdown.find("Measured: 2026-08-06\n\n"));
    EXPECT_NE(std::string::npos, markdown.find("Build: `Debug`\n\n"));
    EXPECT_NE(std::string::npos, markdown.find("Scope: Scope text.\n\n"));
}
