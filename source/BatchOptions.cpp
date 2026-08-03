#include "BatchOptions.hpp"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <vector>

namespace
{

char LowerAscii(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
    {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

bool EqualIgnoreCase(const char *left, const char *right)
{
    if (left == 0 || right == 0)
    {
        return false;
    }
    while (*left != '\0' && *right != '\0')
    {
        if (LowerAscii(*left) != LowerAscii(*right))
        {
            return false;
        }
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

bool EqualText(const char *left, const char *right)
{
    if (left == 0 || right == 0)
    {
        return false;
    }
    return std::string(left) == right;
}

bool EndsWithBmp(const std::string &path)
{
    const std::string suffix = ".bmp";
    if (path.size() < suffix.size())
    {
        return false;
    }
    const std::string::size_type offset = path.size() - suffix.size();
    for (std::string::size_type i = 0; i < suffix.size(); ++i)
    {
        if (LowerAscii(path[offset + i]) != suffix[i])
        {
            return false;
        }
    }
    return true;
}

bool ParseBackend(const char *text, capow::BatchBackend *backend)
{
    if (EqualIgnoreCase(text, "cpu"))
    {
        *backend = capow::BATCH_BACKEND_CPU;
        return true;
    }
    if (EqualIgnoreCase(text, "gpu"))
    {
        *backend = capow::BATCH_BACKEND_GPU;
        return true;
    }
    return false;
}

bool ParseRule(const char *text, capow::BatchRule *rule)
{
    if (EqualIgnoreCase(text, "SYNTHETIC_HEAT_2D"))
    {
        *rule = capow::BATCH_RULE_SYNTHETIC_HEAT_2D;
        return true;
    }
    if (EqualIgnoreCase(text, "CA_HEAT_2D"))
    {
        *rule = capow::BATCH_RULE_CA_HEAT_2D;
        return true;
    }
    if (EqualIgnoreCase(text, "CA_WAVE_2D"))
    {
        *rule = capow::BATCH_RULE_CA_WAVE_2D;
        return true;
    }
    return false;
}

bool ParseWrapMode(const char *text, capow::BatchWrapMode *wrapMode)
{
    if (EqualIgnoreCase(text, "zero"))
    {
        *wrapMode = capow::BATCH_WRAP_ZERO;
        return true;
    }
    if (EqualIgnoreCase(text, "fixed"))
    {
        *wrapMode = capow::BATCH_WRAP_FIXED;
        return true;
    }
    if (EqualIgnoreCase(text, "wrap"))
    {
        *wrapMode = capow::BATCH_WRAP_WRAP;
        return true;
    }
    if (EqualIgnoreCase(text, "free"))
    {
        *wrapMode = capow::BATCH_WRAP_FREE;
        return true;
    }
    if (EqualIgnoreCase(text, "absorb"))
    {
        *wrapMode = capow::BATCH_WRAP_ABSORB;
        return true;
    }
    return false;
}

bool ParsePositiveInt(const char *text, int *value)
{
    char *end = 0;
    long parsed;
    errno = 0;
    parsed = std::strtol(text, &end, 10);
    if (text == 0 || *text == '\0' || *end != '\0' || errno == ERANGE)
    {
        return false;
    }
    if (parsed <= 0 || parsed > INT_MAX)
    {
        return false;
    }
    *value = static_cast<int>(parsed);
    return true;
}

bool ParsePositiveUnsignedLong(const char *text, unsigned long *value)
{
    char *end = 0;
    unsigned long parsed;
    if (text == 0 || *text < '0' || *text > '9')
    {
        return false;
    }
    errno = 0;
    parsed = std::strtoul(text, &end, 10);
    if (*end != '\0' || errno == ERANGE)
    {
        return false;
    }
    if (parsed == 0)
    {
        return false;
    }
    *value = parsed;
    return true;
}

bool HasBatchFlag(int argc, const char *argv[])
{
    for (int i = 0; i < argc; ++i)
    {
        if (EqualText(argv[i], "--batch"))
        {
            return true;
        }
    }
    return false;
}

bool AppendToken(std::vector<std::string> *tokens, std::string *token)
{
    if (!token->empty())
    {
        tokens->push_back(*token);
        token->clear();
    }
    return true;
}

std::vector<std::string> SplitCommandLine(const char *commandLine, std::string *error)
{
    std::vector<std::string> tokens;
    std::string token;
    bool inQuotes = false;
    const char *scan = commandLine;

    if (scan == 0)
    {
        return tokens;
    }

    while (*scan != '\0')
    {
        if (*scan == '"')
        {
            inQuotes = !inQuotes;
        }
        else if ((*scan == ' ' || *scan == '\t') && !inQuotes)
        {
            AppendToken(&tokens, &token);
        }
        else
        {
            token.push_back(*scan);
        }
        ++scan;
    }

    if (inQuotes)
    {
        *error = "unterminated quote in batch command line";
        tokens.clear();
        return tokens;
    }

    AppendToken(&tokens, &token);
    return tokens;
}

void Fail(capow::BatchParseResult *result, const char *error)
{
    result->ok = false;
    result->error = error;
}

bool NeedValue(capow::BatchParseResult *result, int index, int argc, const char *name)
{
    if (index + 1 < argc)
    {
        return true;
    }
    result->ok = false;
    result->error = "missing value for ";
    result->error += name;
    return false;
}

} // namespace

namespace capow
{

BatchOptions::BatchOptions() :
    batch(false),
    backend(BATCH_BACKEND_CPU),
    rule(BATCH_RULE_CA_HEAT_2D),
    wrapMode(BATCH_WRAP_WRAP),
    steps(0),
    seed(1946)
{
}

BatchParseResult::BatchParseResult() :
    ok(true),
    batch(false)
{
}

BatchParseResult ParseBatchArguments(int argc, const char *argv[])
{
    BatchParseResult result;
    bool haveBackend = false;
    bool haveRule = false;
    bool haveWrap = false;
    bool haveSteps = false;
    bool haveSeed = false;
    bool haveOutput = false;

    if (!HasBatchFlag(argc, argv))
    {
        return result;
    }

    result.batch = true;
    result.options.batch = true;

    for (int i = 0; i < argc && result.ok; ++i)
    {
        const char *arg = argv[i];
        if (EqualText(arg, "--batch"))
        {
            continue;
        }
        if (EqualText(arg, "--backend"))
        {
            if (haveBackend)
            {
                Fail(&result, "duplicate --backend");
                continue;
            }
            if (!NeedValue(&result, i, argc, "--backend"))
            {
                continue;
            }
            if (!ParseBackend(argv[++i], &result.options.backend))
            {
                Fail(&result, "invalid backend");
            }
            haveBackend = result.ok;
            continue;
        }
        if (EqualText(arg, "--rule"))
        {
            if (haveRule)
            {
                Fail(&result, "duplicate --rule");
                continue;
            }
            if (!NeedValue(&result, i, argc, "--rule"))
            {
                continue;
            }
            if (!ParseRule(argv[++i], &result.options.rule))
            {
                Fail(&result, "invalid rule");
            }
            haveRule = result.ok;
            continue;
        }
        if (EqualText(arg, "--steps"))
        {
            if (haveSteps)
            {
                Fail(&result, "duplicate --steps");
                continue;
            }
            if (!NeedValue(&result, i, argc, "--steps"))
            {
                continue;
            }
            if (!ParsePositiveInt(argv[++i], &result.options.steps))
            {
                Fail(&result, "invalid step count");
            }
            haveSteps = result.ok;
            continue;
        }
        if (EqualText(arg, "--wrap"))
        {
            if (haveWrap)
            {
                Fail(&result, "duplicate --wrap");
                continue;
            }
            if (!NeedValue(&result, i, argc, "--wrap"))
            {
                continue;
            }
            if (!ParseWrapMode(argv[++i], &result.options.wrapMode))
            {
                Fail(&result, "invalid wrap mode");
            }
            haveWrap = result.ok;
            continue;
        }
        if (EqualText(arg, "--seed"))
        {
            if (haveSeed)
            {
                Fail(&result, "duplicate --seed");
                continue;
            }
            if (!NeedValue(&result, i, argc, "--seed"))
            {
                continue;
            }
            if (!ParsePositiveUnsignedLong(argv[++i], &result.options.seed))
            {
                Fail(&result, "invalid seed");
            }
            haveSeed = result.ok;
            continue;
        }
        if (EqualText(arg, "--output"))
        {
            if (haveOutput)
            {
                Fail(&result, "duplicate --output");
                continue;
            }
            if (!NeedValue(&result, i, argc, "--output"))
            {
                continue;
            }
            result.options.output = argv[++i];
            if (!EndsWithBmp(result.options.output))
            {
                Fail(&result, "batch output must be a .bmp file");
            }
            haveOutput = result.ok;
            continue;
        }

        Fail(&result, "unknown batch option");
    }

    if (result.ok && !haveBackend)
    {
        Fail(&result, "missing --backend");
    }
    if (result.ok && !haveRule)
    {
        Fail(&result, "missing --rule");
    }
    if (result.ok && !haveSteps)
    {
        Fail(&result, "missing --steps");
    }
    if (result.ok && !haveOutput)
    {
        Fail(&result, "missing --output");
    }

    return result;
}

BatchParseResult ParseBatchCommandLine(const char *commandLine)
{
    std::string error;
    std::vector<std::string> tokens = SplitCommandLine(commandLine, &error);
    std::vector<const char *> argv;
    BatchParseResult result;

    if (!error.empty())
    {
        result.ok = false;
        result.batch = true;
        result.error = error;
        return result;
    }

    for (std::vector<std::string>::const_iterator it = tokens.begin(); it != tokens.end(); ++it)
    {
        argv.push_back(it->c_str());
    }

    return ParseBatchArguments(static_cast<int>(argv.size()), argv.data());
}

const char *BatchRuleName(BatchRule rule)
{
    switch (rule)
    {
    case BATCH_RULE_SYNTHETIC_HEAT_2D:
        return "SYNTHETIC_HEAT_2D";
    case BATCH_RULE_CA_HEAT_2D:
        return "CA_HEAT_2D";
    case BATCH_RULE_CA_WAVE_2D:
        return "CA_WAVE_2D";
    }
    return "unknown";
}

} // namespace capow
