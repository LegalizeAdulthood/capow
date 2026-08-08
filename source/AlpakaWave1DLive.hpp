#ifndef ALPAKAWAVE1DLIVE_HPP
#define ALPAKAWAVE1DLIVE_HPP

#include "AlpakaBuffers.hpp"
#include "CapowRules.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace capow
{

enum Wave1DLiveView
{
    WAVE_1D_LIVE_VIEW_DOWN,
    WAVE_1D_LIVE_VIEW_SCROLL
};

struct Wave1DLiveOptions
{
    int width;
    int historyWidth;
    int historyHeight;
    int row;
    int bltLines;
    Wave1DLiveView view;
    Wave1DRule rule;
    AlpakaPlaneValue waveSpeed2TimeStep2OverDx2;
    AlpakaPlaneValue dtOverDx2;
    AlpakaPlaneValue maxIntensity;
    AlpakaPlaneValue maxVelocity;
    AlpakaPlaneValue timeStep;
    AlpakaPlaneValue dtOverMass;
    AlpakaPlaneValue frictionMultiplier;
    AlpakaPlaneValue springMultiplier;
    AlpakaPlaneValue driverValue;
    AlpakaPlaneValue nonlinearity1;
    AlpakaPlaneValue nonlinearity2;
    AlpakaPlaneValue velocityColorScale;
    int colorCount;
    bool showVelocity;

    Wave1DLiveOptions();
};

class Wave1DLiveState
{
public:
    Wave1DLiveState();
    ~Wave1DLiveState();

    Wave1DLiveState(const Wave1DLiveState &) = delete;
    Wave1DLiveState &operator=(const Wave1DLiveState &) = delete;

    bool IsActive() const;
    bool NeedsSource(const Wave1DLiveOptions &options) const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool DownloadCurrentAndPast(AlpakaPlaneValue *targetIntensity, int intensityStride,
        AlpakaPlaneValue *targetVelocity, int velocityStride, AlpakaPlaneValue *pastIntensity, int pastStride,
        AlpakaPlaneValue *nonlinearityTweaks, int nonlinearityStride, std::string *error);
    bool RunFrame(const Wave1DLiveOptions &options, const AlpakaPlaneValue *sourceIntensity, int intensityStride,
        const AlpakaPlaneValue *pastIntensity, int pastStride, const AlpakaPlaneValue *sourceVelocity,
        int velocityStride, const AlpakaPlaneValue *frictionTweaks, const AlpakaPlaneValue *springTweaks,
        const AlpakaPlaneValue *massTweaks, const AlpakaPlaneValue *nonlinearityTweaks, const std::uint32_t *colorTable,
        std::string *error);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace capow

#endif
