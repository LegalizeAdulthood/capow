#include "AlpakaBackend.hpp"

#include <alpaka/alpaka.hpp>

#include <cstddef>
#include <cstdint>

namespace
{

bool DetectGpuAvailable()
{
#if defined(ALPAKA_ACC_GPU_CUDA_ENABLED)
    try
    {
        using Dim = alpaka::DimInt<1U>;
        using Idx = std::uint32_t;
        using Acc = alpaka::AccGpuCudaRt<Dim, Idx>;

        const alpaka::Platform<Acc> platform = alpaka::Platform<Acc>{};
        const std::size_t deviceCount = alpaka::getDevCount(platform);
        if (deviceCount == 0U)
        {
            return false;
        }
        (void) alpaka::getDevByIdx(platform, 0U);
        return true;
    }
    catch (...)
    {
        return false;
    }
#else
    return false;
#endif
}

bool IsRuleInRange(capow::AlpakaRule rule)
{
    const int ruleIndex = static_cast<int>(rule);
    return ruleIndex >= 0 && ruleIndex < static_cast<int>(capow::ALPAKA_RULE_COUNT);
}

capow::AlpakaManager gAlpakaManager;

} // namespace

namespace capow
{

AlpakaManager::AlpakaManager() :
    gpuAvailable(DetectGpuAvailable()),
    backend(ALPAKA_BACKEND_CPU),
    availabilityMessage(gpuAvailable ? "alpaka CUDA device available" : "alpaka CUDA device unavailable")
{
    for (int ruleIndex = 0; ruleIndex < static_cast<int>(ALPAKA_RULE_COUNT); ++ruleIndex)
    {
        ruleEnabled[ruleIndex] = false;
    }
    ruleEnabled[ALPAKA_RULE_SYNTHETIC_HEAT_2D] = true;
    ruleEnabled[ALPAKA_RULE_CA_HEAT_2D] = true;
    ruleEnabled[ALPAKA_RULE_CA_WAVE_2D] = true;
    ruleEnabled[ALPAKA_RULE_CA_HEATWAVE] = true;
    ruleEnabled[ALPAKA_RULE_CA_HEATWAVE2] = true;
}

bool AlpakaManager::IsGpuAvailable() const
{
    return gpuAvailable;
}

AlpakaBackend AlpakaManager::GetBackend() const
{
    return backend;
}

void AlpakaManager::SetBackend(AlpakaBackend nextBackend)
{
    backend = nextBackend;
}

bool AlpakaManager::IsRuleEnabled(AlpakaRule rule) const
{
    if (!IsRuleInRange(rule))
    {
        return false;
    }
    return ruleEnabled[static_cast<int>(rule)];
}

void AlpakaManager::SetRuleEnabled(AlpakaRule rule, bool enabled)
{
    if (IsRuleInRange(rule))
    {
        ruleEnabled[static_cast<int>(rule)] = enabled;
    }
}

bool AlpakaManager::CanRunGpu(AlpakaRule rule) const
{
    return gpuAvailable && IsRuleEnabled(rule);
}

const char *AlpakaManager::GetAvailabilityMessage() const
{
    return availabilityMessage;
}

AlpakaManager &GetAlpakaManager()
{
    return gAlpakaManager;
}

const char *AlpakaBackendName(AlpakaBackend backend)
{
    switch (backend)
    {
    case ALPAKA_BACKEND_CPU:
        return "CPU";
    case ALPAKA_BACKEND_GPU:
        return "GPU";
    }
    return "unknown";
}

const char *AlpakaRuleName(AlpakaRule rule)
{
    switch (rule)
    {
    case ALPAKA_RULE_SYNTHETIC_HEAT_2D:
        return "SYNTHETIC_HEAT_2D";
    case ALPAKA_RULE_CA_HEAT_2D:
        return "CA_HEAT_2D";
    case ALPAKA_RULE_CA_WAVE_2D:
        return "CA_WAVE_2D";
    case ALPAKA_RULE_CA_HEATWAVE:
        return "CA_HEATWAVE";
    case ALPAKA_RULE_CA_HEATWAVE2:
        return "CA_HEATWAVE2";
    case ALPAKA_RULE_COUNT:
        break;
    }
    return "unknown";
}

} // namespace capow
