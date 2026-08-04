#include "../source/WavePlaneImage.hpp"

#include "../source/ca.hpp"

#include <gtest/gtest.h>

#include <array>

namespace
{

using capow::ImageBuffer;

ImageBuffer::Pixel PixelAt(const ImageBuffer &buffer, int x, int y)
{
    return buffer
        .Data()[static_cast<std::size_t>(y) * static_cast<std::size_t>(buffer.Width()) + static_cast<std::size_t>(x)];
}

} // namespace

TEST(wavePlaneImage, imagePixelFromColorRefUsesBgraMemoryOrder)
{
    const ImageBuffer::Pixel pixel = capow::ImagePixelFromColorRef(RGB(0x12, 0x34, 0x56));
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(&pixel);

    EXPECT_EQ(0xFF123456U, pixel);
    EXPECT_EQ(0x56, bytes[0]);
    EXPECT_EQ(0x34, bytes[1]);
    EXPECT_EQ(0x12, bytes[2]);
    EXPECT_EQ(0xFF, bytes[3]);
}

TEST(wavePlaneImage, renderMapsIntensityThroughColorTable)
{
    std::array<COLORREF, MAX_COLOR> colorTable = {};
    colorTable[0] = RGB(10, 20, 30);
    colorTable[MAX_COLOR - 1] = RGB(40, 50, 60);

    std::array<Wavecell2, 4> plane = {};
    plane[0].variable[0] = -1.0f;
    plane[1].variable[0] = 1.0f;

    ImageBuffer image;
    capow::RenderWavePlaneToImageBuffer(&image, plane.data(), 2, 1, 4, colorTable.data(), 1.0f, false);

    EXPECT_EQ(capow::ImagePixelFromColorRef(colorTable[0]), PixelAt(image, 0, 0));
    EXPECT_EQ(capow::ImagePixelFromColorRef(colorTable[MAX_COLOR - 1]), PixelAt(image, 1, 0));
}

TEST(wavePlaneImage, renderMapsVelocityWhenRequested)
{
    std::array<COLORREF, MAX_COLOR> colorTable = {};
    colorTable[0] = RGB(10, 20, 30);
    colorTable[MAX_COLOR - 1] = RGB(40, 50, 60);

    std::array<Wavecell2, 1> plane = {};
    plane[0].variable[0] = -1.0f;
    plane[0].variable[1] = 1.0f / AMPLIFY_VEL_COLOR_2D;

    ImageBuffer image;
    capow::RenderWavePlaneToImageBuffer(&image, plane.data(), 1, 1, 1, colorTable.data(), 1.0f, true);

    EXPECT_EQ(capow::ImagePixelFromColorRef(colorTable[MAX_COLOR - 1]), PixelAt(image, 0, 0));
}
