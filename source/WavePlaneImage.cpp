#include "WavePlaneImage.hpp"

#include "ca.hpp"

namespace capow
{

ImageBuffer::Pixel ImagePixelFromColorRef(COLORREF color)
{
    return 0xFF000000U | (static_cast<ImageBuffer::Pixel>(GetRValue(color)) << 16U) |
        (static_cast<ImageBuffer::Pixel>(GetGValue(color)) << 8U) | static_cast<ImageBuffer::Pixel>(GetBValue(color));
}

unsigned short WavePlaneColorIndex(Real cellIntensity, Real cellVelocity, Real maxIntensity, bool showVelocity)
{
    const Real displayValue = showVelocity ? AMPLIFY_VEL_COLOR_2D * cellVelocity : cellIntensity;
    unsigned short colorIndex =
        static_cast<unsigned short>(((MAX_COLOR - 1) * (displayValue + maxIntensity)) / (2.0 * maxIntensity));
    POSITIVECLAMP(colorIndex, static_cast<unsigned short>(MAX_COLOR - 1));
    return colorIndex;
}

void RenderWavePlaneToImageBuffer(ImageBuffer *image, const Wavecell2 *plane, int width, int height, int stride,
    const COLORREF *colorTable, Real maxIntensity, bool showVelocity)
{
    if (image->Width() != width || image->Height() != height)
        image->Resize(width, height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const Wavecell2 &cell = plane[y * stride + x];
            const unsigned short colorIndex =
                WavePlaneColorIndex(cell.variable[0], cell.variable[1], maxIntensity, showVelocity);
            image->PutPixel(x, y, ImagePixelFromColorRef(colorTable[colorIndex]));
        }
    }
}

bool CopyImageBufferToDevice(HDC hdc, const ImageBuffer &image, int left, int top)
{
    if (!image.Data())
        return false;

    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = image.Width();
    info.bmiHeader.biHeight = -image.Height();
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    const int copiedLines = SetDIBitsToDevice(
        hdc, left, top, image.Width(), image.Height(), 0, 0, 0, image.Height(), image.Data(), &info, DIB_RGB_COLORS);
    return copiedLines != 0;
}

} // namespace capow
