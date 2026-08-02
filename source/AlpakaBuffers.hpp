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

} // namespace capow

#endif
