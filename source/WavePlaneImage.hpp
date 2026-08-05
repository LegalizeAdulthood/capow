#ifndef WAVEPLANEIMAGE_HPP
#define WAVEPLANEIMAGE_HPP

#include "ImageBuffer.hpp"
#include "Types.h"

struct Wavecell2;

namespace capow
{

ImageBuffer::Pixel ImagePixelFromColorRef(COLORREF color);
unsigned short WavePlaneColorIndex(Real cellIntensity, Real cellVelocity, Real maxIntensity, bool showVelocity);
void RenderWavePlaneToImageBuffer(ImageBuffer *image, const Wavecell2 *plane, int width, int height, int stride,
    const COLORREF *colorTable, Real maxIntensity, bool showVelocity);
bool CopyImageBufferToDevice(HDC hdc, const ImageBuffer &image, int left, int top);
bool CopyImageBufferToDevice(HDC hdc, const ImageBuffer &image, int left, int top, int width, int height);

} // namespace capow

#endif
