#include "ImageBuffer.hpp"

#include <algorithm>
#include <cstring>

namespace
{

bool ClipRect(int bufferWidth, int bufferHeight, int *left, int *top, int *width, int *height)
{
    if (bufferWidth <= 0 || bufferHeight <= 0 || *width <= 0 || *height <= 0)
        return false;

    int right = *left + *width;
    int bottom = *top + *height;
    *left = std::max(*left, 0);
    *top = std::max(*top, 0);
    right = std::min(right, bufferWidth);
    bottom = std::min(bottom, bufferHeight);
    *width = right - *left;
    *height = bottom - *top;
    return *width > 0 && *height > 0;
}

} // namespace

namespace capow
{

ImageBuffer::ImageBuffer() :
    width(0),
    height(0)
{
}

ImageBuffer::ImageBuffer(int width, int height) :
    width(0),
    height(0)
{
    Resize(width, height);
}

void ImageBuffer::Resize(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0)
    {
        width = 0;
        height = 0;
        pixels.clear();
        return;
    }

    width = newWidth;
    height = newHeight;
    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), Pixel(0));
}

void ImageBuffer::Clear(Pixel pixel)
{
    std::fill(pixels.begin(), pixels.end(), pixel);
}

void ImageBuffer::PutPixel(int x, int y, Pixel pixel)
{
    if (x < 0 || y < 0 || x >= width || y >= height)
        return;

    pixels[Index(x, y)] = pixel;
}

void ImageBuffer::FillRect(int left, int top, int fillWidth, int fillHeight, Pixel pixel)
{
    if (!ClipRect(width, height, &left, &top, &fillWidth, &fillHeight))
        return;

    for (int y = top; y < top + fillHeight; ++y)
    {
        std::fill(pixels.begin() + Index(left, y), pixels.begin() + Index(left + fillWidth, y), pixel);
    }
}

void ImageBuffer::ScrollRect(int left, int top, int scrollWidth, int scrollHeight, int deltaX, int deltaY)
{
    if (!ClipRect(width, height, &left, &top, &scrollWidth, &scrollHeight))
        return;

    int sourceLeft = left;
    int sourceTop = top;
    int destinationLeft = left;
    int destinationTop = top;
    int copyWidth = scrollWidth;
    int copyHeight = scrollHeight;

    if (deltaX < 0)
    {
        sourceLeft -= deltaX;
        copyWidth += deltaX;
    }
    else if (deltaX > 0)
    {
        destinationLeft += deltaX;
        copyWidth -= deltaX;
    }

    if (deltaY < 0)
    {
        sourceTop -= deltaY;
        copyHeight += deltaY;
    }
    else if (deltaY > 0)
    {
        destinationTop += deltaY;
        copyHeight -= deltaY;
    }

    if (copyWidth <= 0 || copyHeight <= 0)
        return;

    const std::size_t rowBytes = static_cast<std::size_t>(copyWidth) * sizeof(Pixel);
    if (deltaY > 0)
    {
        for (int row = copyHeight - 1; row >= 0; --row)
        {
            std::memmove(&pixels[Index(destinationLeft, destinationTop + row)],
                &pixels[Index(sourceLeft, sourceTop + row)], rowBytes);
        }
    }
    else
    {
        for (int row = 0; row < copyHeight; ++row)
        {
            std::memmove(&pixels[Index(destinationLeft, destinationTop + row)],
                &pixels[Index(sourceLeft, sourceTop + row)], rowBytes);
        }
    }
}

ImageBuffer::Pixel *ImageBuffer::Data()
{
    if (pixels.empty())
        return 0;
    return pixels.data();
}

const ImageBuffer::Pixel *ImageBuffer::Data() const
{
    if (pixels.empty())
        return 0;
    return pixels.data();
}

int ImageBuffer::Width() const
{
    return width;
}

int ImageBuffer::Height() const
{
    return height;
}

std::size_t ImageBuffer::Pitch() const
{
    return static_cast<std::size_t>(width) * sizeof(Pixel);
}

std::size_t ImageBuffer::Index(int x, int y) const
{
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

} // namespace capow
