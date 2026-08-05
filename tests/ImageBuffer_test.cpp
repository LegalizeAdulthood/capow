#include "../source/ImageBuffer.hpp"

#include <gtest/gtest.h>

namespace
{

using capow::ImageBuffer;

ImageBuffer::Pixel PixelAt(const ImageBuffer &buffer, int x, int y)
{
    return buffer
        .Data()[static_cast<std::size_t>(y) * static_cast<std::size_t>(buffer.Width()) + static_cast<std::size_t>(x)];
}

} // namespace

TEST(imageBuffer, resizeSetsDimensionsAndPitch)
{
    ImageBuffer buffer(3, 2);

    EXPECT_EQ(3, buffer.Width());
    EXPECT_EQ(2, buffer.Height());
    EXPECT_EQ(3U * sizeof(ImageBuffer::Pixel), buffer.Pitch());
    ASSERT_NE(nullptr, buffer.Data());
    EXPECT_EQ(0U, PixelAt(buffer, 0, 0));
    EXPECT_EQ(0U, PixelAt(buffer, 2, 1));

    buffer.Resize(0, 3);

    EXPECT_EQ(0, buffer.Width());
    EXPECT_EQ(0, buffer.Height());
    EXPECT_EQ(0U, buffer.Pitch());
    EXPECT_EQ(nullptr, buffer.Data());
}

TEST(imageBuffer, clearSetsEveryPixel)
{
    ImageBuffer buffer(4, 3);

    buffer.Clear(0x11223344U);

    for (int y = 0; y < buffer.Height(); ++y)
    {
        for (int x = 0; x < buffer.Width(); ++x)
            EXPECT_EQ(0x11223344U, PixelAt(buffer, x, y));
    }
}

TEST(imageBuffer, putPixelIgnoresOutOfBoundsCoordinates)
{
    ImageBuffer buffer(2, 2);

    buffer.Clear(1U);
    buffer.PutPixel(1, 0, 9U);
    buffer.PutPixel(-1, 0, 5U);
    buffer.PutPixel(2, 0, 6U);
    buffer.PutPixel(0, -1, 7U);
    buffer.PutPixel(0, 2, 8U);

    EXPECT_EQ(1U, PixelAt(buffer, 0, 0));
    EXPECT_EQ(9U, PixelAt(buffer, 1, 0));
    EXPECT_EQ(1U, PixelAt(buffer, 0, 1));
    EXPECT_EQ(1U, PixelAt(buffer, 1, 1));
}

TEST(imageBuffer, fillRectClipsToBufferBounds)
{
    ImageBuffer buffer(4, 3);

    buffer.Clear(1U);
    buffer.FillRect(-1, 1, 3, 4, 7U);

    EXPECT_EQ(1U, PixelAt(buffer, 0, 0));
    EXPECT_EQ(1U, PixelAt(buffer, 1, 0));
    EXPECT_EQ(7U, PixelAt(buffer, 0, 1));
    EXPECT_EQ(7U, PixelAt(buffer, 1, 1));
    EXPECT_EQ(1U, PixelAt(buffer, 2, 1));
    EXPECT_EQ(7U, PixelAt(buffer, 0, 2));
    EXPECT_EQ(7U, PixelAt(buffer, 1, 2));
    EXPECT_EQ(1U, PixelAt(buffer, 2, 2));
}

TEST(imageBuffer, scrollRectMovesPixelsWithinClippedRegion)
{
    ImageBuffer buffer(4, 4);

    for (int y = 0; y < buffer.Height(); ++y)
    {
        for (int x = 0; x < buffer.Width(); ++x)
        {
            buffer.PutPixel(x, y, static_cast<ImageBuffer::Pixel>(y * buffer.Width() + x));
        }
    }

    buffer.ScrollRect(1, 1, 3, 3, -1, -1);

    EXPECT_EQ(0U, PixelAt(buffer, 0, 0));
    EXPECT_EQ(10U, PixelAt(buffer, 1, 1));
    EXPECT_EQ(11U, PixelAt(buffer, 2, 1));
    EXPECT_EQ(14U, PixelAt(buffer, 1, 2));
    EXPECT_EQ(15U, PixelAt(buffer, 2, 2));
    EXPECT_EQ(15U, PixelAt(buffer, 3, 3));
}

TEST(imageBuffer, scrollRectUsesOverlapSafeCopy)
{
    ImageBuffer buffer(4, 3);

    for (int y = 0; y < buffer.Height(); ++y)
    {
        for (int x = 0; x < buffer.Width(); ++x)
        {
            buffer.PutPixel(x, y, static_cast<ImageBuffer::Pixel>(y * buffer.Width() + x));
        }
    }

    buffer.ScrollRect(0, 0, 4, 3, 1, 1);

    EXPECT_EQ(0U, PixelAt(buffer, 1, 1));
    EXPECT_EQ(1U, PixelAt(buffer, 2, 1));
    EXPECT_EQ(4U, PixelAt(buffer, 1, 2));
    EXPECT_EQ(5U, PixelAt(buffer, 2, 2));
}

TEST(imageBuffer, scrollRectMovesHistoryRowsUp)
{
    ImageBuffer buffer(3, 5);

    for (int y = 0; y < buffer.Height(); ++y)
    {
        for (int x = 0; x < buffer.Width(); ++x)
        {
            buffer.PutPixel(x, y, static_cast<ImageBuffer::Pixel>(10 * y + x));
        }
    }

    buffer.ScrollRect(0, 1, 3, 4, 0, -2);

    EXPECT_EQ(30U, PixelAt(buffer, 0, 1));
    EXPECT_EQ(31U, PixelAt(buffer, 1, 1));
    EXPECT_EQ(40U, PixelAt(buffer, 0, 2));
    EXPECT_EQ(41U, PixelAt(buffer, 1, 2));
    EXPECT_EQ(30U, PixelAt(buffer, 0, 3));
    EXPECT_EQ(40U, PixelAt(buffer, 0, 4));
}
