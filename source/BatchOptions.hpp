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

struct BatchOptions
{
    bool batch;
    BatchBackend backend;
    BatchRule rule;
    int steps;
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
