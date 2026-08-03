#ifndef ALPAKAHEAT2DLIVE_HPP
#define ALPAKAHEAT2DLIVE_HPP

#include "AlpakaBuffers.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace capow
{

struct Heat2DLiveOptions
{
    int width;
    int height;
    AlpakaPlaneValue heatIncrement;
    AlpakaPlaneValue maxIntensity;
    AlpakaPlaneValue timeStep;
    AlpakaPlaneValue velocityColorScale;
    int colorCount;
    bool showVelocity;
};

class Heat2DLiveState
{
public:
    Heat2DLiveState();
    ~Heat2DLiveState();

    Heat2DLiveState(const Heat2DLiveState &) = delete;
    Heat2DLiveState &operator=(const Heat2DLiveState &) = delete;

    bool IsActive() const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool RunFrame(const Heat2DLiveOptions &options, const AlpakaPlaneValue *sourcePlane, int valueStride,
        const std::uint32_t *colorTable, std::string *error);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace capow

#endif
