#ifndef ALPAKADIGITALLIVE_HPP
#define ALPAKADIGITALLIVE_HPP

#include "AlpakaBuffers.hpp"

#include <cstddef>
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

enum Digital1DLiveRule
{
    DIGITAL_1D_LIVE_RULE_STANDARD,
    DIGITAL_1D_LIVE_RULE_REVERSIBLE
};

struct Digital1DLiveOptions
{
    int width;
    int historyWidth;
    int historyHeight;
    int row;
    int bltLines;
    Digital1DLiveView view;
    Digital1DLiveRule rule;
    int radius;
    int stateBits;
    int stateCount;
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
    bool DownloadPast(AlpakaDigitalValue *pastRow, std::string *error);
    bool DownloadHistory(std::uint32_t *historyPixels, std::size_t count, std::string *error);
    bool RunFrame(const Digital1DLiveOptions &options, const AlpakaDigitalValue *sourceRow,
        const AlpakaDigitalValue *pastRow, const AlpakaDigitalValue *lookup, const std::uint32_t *colorTable,
        const std::uint32_t *historyPixels, std::string *error);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace capow

#endif
