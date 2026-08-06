#ifndef ALPAKAHEAT2DLIVE_HPP
#define ALPAKAHEAT2DLIVE_HPP

#include "AlpakaBuffers.hpp"
#include "CapowRules.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace capow
{

enum Live2DRule
{
    LIVE_2D_RULE_HEAT,
    LIVE_2D_RULE_WAVE
};

struct Heat2DLiveOptions
{
    int width;
    int height;
    Live2DRule rule;
    AlpakaPlaneValue heatIncrement;
    AlpakaPlaneValue waveSpeed2TimeStep2OverDx2;
    AlpakaPlaneValue maxIntensity;
    AlpakaPlaneValue timeStep;
    AlpakaPlaneValue velocityColorScale;
    int colorCount;
    bool showVelocity;
    Heat2DBoundaryMode boundaryMode;
};

class Heat2DLiveState
{
public:
    Heat2DLiveState();
    ~Heat2DLiveState();

    Heat2DLiveState(const Heat2DLiveState &) = delete;
    Heat2DLiveState &operator=(const Heat2DLiveState &) = delete;

    bool IsActive() const;
    bool NeedsSource(const Heat2DLiveOptions &options) const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool DownloadCurrent(AlpakaPlaneValue *targetPlane, int valueStride, AlpakaPlaneValue *targetVelocity,
        int velocityStride, std::string *error);
    bool DownloadCurrentAndPast(AlpakaPlaneValue *targetPlane, int valueStride, AlpakaPlaneValue *targetVelocity,
        int velocityStride, AlpakaPlaneValue *pastPlane, int pastStride, std::string *error);
    bool RunFrame(const Heat2DLiveOptions &options, const AlpakaPlaneValue *sourcePlane, int valueStride,
        const std::uint32_t *colorTable, std::string *error);
    bool RunFrame(const Heat2DLiveOptions &options, const AlpakaPlaneValue *sourcePlane, int valueStride,
        const AlpakaPlaneValue *pastPlane, int pastStride, const std::uint32_t *colorTable, std::string *error);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace capow

#endif
