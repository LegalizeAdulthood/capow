#ifndef BATCHOPTIONS_HPP
#define BATCHOPTIONS_HPP

#include <string>

namespace capow
{

enum BatchBackend
{
    BATCH_BACKEND_CPU,
    BATCH_BACKEND_GPU
};

enum BatchRule
{
    BATCH_RULE_SYNTHETIC_HEAT_2D,
    BATCH_RULE_CA_HEAT_2D,
    BATCH_RULE_CA_WAVE_2D,
    BATCH_RULE_CA_HEATWAVE,
    BATCH_RULE_CA_HEATWAVE2,
    BATCH_RULE_CA_WAVE,
    BATCH_RULE_CA_WAVE2,
    BATCH_RULE_CA_OSCILLATOR,
    BATCH_RULE_CA_DIVERSE_OSCILLATOR,
    BATCH_RULE_ALT_CA_OSCILLATOR_WAVE,
    BATCH_RULE_ALT_CA_DIVERSE_OSCILLATOR_WAVE,
    BATCH_RULE_CA_ULAM_WAVE,
    BATCH_RULE_CA_AUTO_ULAM_WAVE,
    BATCH_RULE_CA_CUBIC_ULAM_WAVE,
    BATCH_RULE_CA_STANDARD,
    BATCH_RULE_CA_REVERSIBLE
};

enum BatchWrapMode
{
    BATCH_WRAP_ZERO,
    BATCH_WRAP_FIXED,
    BATCH_WRAP_WRAP,
    BATCH_WRAP_FREE,
    BATCH_WRAP_ABSORB
};

struct BatchOptions
{
    bool batch;
    BatchBackend backend;
    BatchRule rule;
    BatchWrapMode wrapMode;
    int steps;
    unsigned long seed;
    std::string output;

    BatchOptions();
};

struct BatchParseResult
{
    bool ok;
    bool batch;
    BatchOptions options;
    std::string error;

    BatchParseResult();
};

BatchParseResult ParseBatchArguments(int argc, const char *argv[]);
BatchParseResult ParseBatchCommandLine(const char *commandLine);
const char *BatchRuleName(BatchRule rule);

} // namespace capow

#endif
