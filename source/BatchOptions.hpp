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
    BATCH_RULE_CA_WAVE_2D
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
