#ifndef ALPAKADIGITALLIVE_HPP
#define ALPAKADIGITALLIVE_HPP

#include "AlpakaBuffers.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace capow
{

enum Digital1DLiveView
{
    DIGITAL_1D_LIVE_VIEW_DOWN,
    DIGITAL_1D_LIVE_VIEW_SCROLL
};

struct Digital1DLiveOptions
{
    int width;
    int historyWidth;
    int historyHeight;
    int row;
    int bltLines;
    Digital1DLiveView view;
    int radius;
    int stateBits;
    int lookupCount;
    int colorCount;

    Digital1DLiveOptions();
};

class Digital1DLiveState
{
public:
    Digital1DLiveState();
    ~Digital1DLiveState();

    Digital1DLiveState(const Digital1DLiveState &) = delete;
    Digital1DLiveState &operator=(const Digital1DLiveState &) = delete;

    bool IsActive() const;
    bool NeedsSource(const Digital1DLiveOptions &options) const;
    unsigned int GetTexture() const;
    void Deactivate();
    bool DownloadCurrent(AlpakaDigitalValue *targetRow, std::string *error);
    bool RunFrame(const Digital1DLiveOptions &options, const AlpakaDigitalValue *sourceRow,
        const AlpakaDigitalValue *lookup, const std::uint32_t *colorTable, std::string *error);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace capow

#endif
