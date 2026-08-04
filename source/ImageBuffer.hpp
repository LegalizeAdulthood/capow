#ifndef IMAGEBUFFER_HPP
#define IMAGEBUFFER_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace capow
{

class ImageBuffer
{
public:
    using Pixel = std::uint32_t;

    ImageBuffer();
    ImageBuffer(int width, int height);

    void Resize(int width, int height);
    void Clear(Pixel pixel);
    void PutPixel(int x, int y, Pixel pixel);
    void FillRect(int left, int top, int width, int height, Pixel pixel);
    void ScrollRect(int left, int top, int width, int height, int deltaX, int deltaY);

    Pixel *Data();
    const Pixel *Data() const;
    int Width() const;
    int Height() const;
    std::size_t Pitch() const;

private:
    std::size_t Index(int x, int y) const;

    int width;
    int height;
    std::vector<Pixel> pixels;
};

} // namespace capow

#endif
