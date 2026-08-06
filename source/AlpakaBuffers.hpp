#ifndef ALPAKABUFFERS_HPP
#define ALPAKABUFFERS_HPP

#include <cstddef>
#include <memory>

namespace capow
{

using AlpakaPlaneValue = float;

class AlpakaPlaneMirror2D
{
public:
    AlpakaPlaneMirror2D();
    ~AlpakaPlaneMirror2D();

    AlpakaPlaneMirror2D(const AlpakaPlaneMirror2D &) = delete;
    AlpakaPlaneMirror2D &operator=(const AlpakaPlaneMirror2D &) = delete;

    bool IsInitialized() const;
    bool IsDirty() const;
    int GetWidth() const;
    int GetHeight() const;
    std::size_t GetCellCount() const;

    void Resize(int width, int height);
    void MarkDirty();
    void CopySourceAndPastToDevice(
        const AlpakaPlaneValue *sourcePlane, const AlpakaPlaneValue *pastPlane, int valueStride);
    void DebugCopySourceToTarget();
    void DebugCopyPastToTarget();
    void CopyTargetToHost(AlpakaPlaneValue *targetPlane, int valueStride);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

class AlpakaContinuousRowMirror1D
{
public:
    AlpakaContinuousRowMirror1D();
    ~AlpakaContinuousRowMirror1D();

    AlpakaContinuousRowMirror1D(const AlpakaContinuousRowMirror1D &) = delete;
    AlpakaContinuousRowMirror1D &operator=(const AlpakaContinuousRowMirror1D &) = delete;

    bool IsInitialized() const;
    bool IsDirty() const;
    int GetWidth() const;
    std::size_t GetCellCount() const;
    int GetSourceSlot() const;
    int GetTargetSlot() const;
    int GetPastSlot() const;

    void Resize(int width);
    void MarkDirty();
    void CopySourceAndPastToDevice(const AlpakaPlaneValue *sourceIntensity, const AlpakaPlaneValue *sourceVelocity,
        const AlpakaPlaneValue *pastIntensity, const AlpakaPlaneValue *pastVelocity, int valueStride);
    void DebugCopySourceToTarget();
    void DebugCopyPastToTarget();
    void CopyTargetToDisplayRow();
    void RotateRows();
    void CopyTargetToHost(AlpakaPlaneValue *targetIntensity, AlpakaPlaneValue *targetVelocity, int valueStride);
    void CopyDisplayRowToHost(AlpakaPlaneValue *displayIntensity, AlpakaPlaneValue *displayVelocity, int valueStride);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace capow

#endif
