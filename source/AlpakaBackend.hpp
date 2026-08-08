#ifndef ALPAKABACKEND_HPP
#define ALPAKABACKEND_HPP

namespace capow
{

enum AlpakaBackend
{
    ALPAKA_BACKEND_CPU,
    ALPAKA_BACKEND_GPU
};

enum AlpakaRule
{
    ALPAKA_RULE_SYNTHETIC_HEAT_2D,
    ALPAKA_RULE_CA_HEAT_2D,
    ALPAKA_RULE_CA_WAVE_2D,
    ALPAKA_RULE_CA_HEATWAVE,
    ALPAKA_RULE_CA_HEATWAVE2,
    ALPAKA_RULE_CA_WAVE,
    ALPAKA_RULE_CA_WAVE2,
    ALPAKA_RULE_CA_OSCILLATOR,
    ALPAKA_RULE_CA_DIVERSE_OSCILLATOR,
    ALPAKA_RULE_ALT_CA_OSCILLATOR_WAVE,
    ALPAKA_RULE_ALT_CA_DIVERSE_OSCILLATOR_WAVE,
    ALPAKA_RULE_CA_ULAM_WAVE,
    ALPAKA_RULE_CA_AUTO_ULAM_WAVE,
    ALPAKA_RULE_CA_CUBIC_ULAM_WAVE,
    ALPAKA_RULE_CA_STANDARD,
    ALPAKA_RULE_COUNT
};

class AlpakaManager
{
public:
    AlpakaManager();

    bool IsGpuAvailable() const;
    AlpakaBackend GetBackend() const;
    void SetBackend(AlpakaBackend nextBackend);
    bool IsRuleEnabled(AlpakaRule rule) const;
    void SetRuleEnabled(AlpakaRule rule, bool enabled);
    bool CanRunGpu(AlpakaRule rule) const;
    const char *GetAvailabilityMessage() const;

private:
    bool gpuAvailable;
    AlpakaBackend backend;
    bool ruleEnabled[ALPAKA_RULE_COUNT];
    const char *availabilityMessage;
};

AlpakaManager &GetAlpakaManager();
const char *AlpakaBackendName(AlpakaBackend backend);
const char *AlpakaRuleName(AlpakaRule rule);

} // namespace capow

#endif
