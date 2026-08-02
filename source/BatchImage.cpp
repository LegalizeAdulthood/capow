#include "BatchImage.hpp"

#include <algorithm>
#include <fstream>
#include <vector>

namespace
{

void WriteU16(std::ofstream *output, unsigned int value)
{
    output->put(static_cast<char>(value & 0xFFU));
    output->put(static_cast<char>((value >> 8U) & 0xFFU));
}

void WriteU32(std::ofstream *output, unsigned int value)
{
    output->put(static_cast<char>(value & 0xFFU));
    output->put(static_cast<char>((value >> 8U) & 0xFFU));
    output->put(static_cast<char>((value >> 16U) & 0xFFU));
    output->put(static_cast<char>((value >> 24U) & 0xFFU));
}

void WriteBmpHeader(std::ofstream *output, int width, int height, int rowStride)
{
    const unsigned int pixelOffset = 54U;
    const unsigned int imageSize = static_cast<unsigned int>(rowStride * height);
    const unsigned int fileSize = pixelOffset + imageSize;

    output->put('B');
    output->put('M');
    WriteU32(output, fileSize);
    WriteU16(output, 0U);
    WriteU16(output, 0U);
    WriteU32(output, pixelOffset);
    WriteU32(output, 40U);
    WriteU32(output, static_cast<unsigned int>(width));
    WriteU32(output, static_cast<unsigned int>(height));
    WriteU16(output, 1U);
    WriteU16(output, 24U);
    WriteU32(output, 0U);
    WriteU32(output, imageSize);
    WriteU32(output, 0U);
    WriteU32(output, 0U);
    WriteU32(output, 0U);
    WriteU32(output, 0U);
}

void CleanupDib(HDC dibDc, HBITMAP dibBitmap, HGDIOBJ oldBitmap)
{
    if (dibDc != 0 && oldBitmap != 0)
    {
        SelectObject(dibDc, oldBitmap);
    }
    if (dibBitmap != 0)
    {
        DeleteObject(dibBitmap);
    }
    if (dibDc != 0)
    {
        DeleteDC(dibDc);
    }
}

unsigned char IntensityToByte(float value)
{
    if (value <= 0.0F)
    {
        return 0U;
    }
    if (value >= 1.0F)
    {
        return 255U;
    }
    return static_cast<unsigned char>(value * 255.0F + 0.5F);
}

} // namespace

namespace capow
{

bool WriteBmpFromHdc(HDC hdc, int left, int top, int width, int height, const char *outputPath, std::string *error)
{
    if (hdc == 0)
    {
        *error = "missing source HDC";
        return false;
    }
    if (width <= 0 || height <= 0)
    {
        *error = "invalid image dimensions";
        return false;
    }
    if (outputPath == 0 || *outputPath == '\0')
    {
        *error = "missing output path";
        return false;
    }

    const int rowBytes = width * 3;
    const int rowStride = (rowBytes + 3) & ~3;
    BITMAPINFO bitmapInfo;
    void *bits = 0;
    ZeroMemory(&bitmapInfo, sizeof(bitmapInfo));
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    const HDC dibDc = CreateCompatibleDC(hdc);
    if (dibDc == 0)
    {
        *error = "could not create batch image DC";
        return false;
    }
    const HBITMAP dibBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &bits, 0, 0);
    if (dibBitmap == 0 || bits == 0)
    {
        CleanupDib(dibDc, dibBitmap, 0);
        *error = "could not create batch image bitmap";
        return false;
    }
    const HGDIOBJ oldBitmap = SelectObject(dibDc, dibBitmap);
    if (oldBitmap == 0)
    {
        CleanupDib(dibDc, dibBitmap, 0);
        *error = "could not select batch image bitmap";
        return false;
    }
    if (!BitBlt(dibDc, 0, 0, width, height, hdc, left, top, SRCCOPY))
    {
        CleanupDib(dibDc, dibBitmap, oldBitmap);
        *error = "could not copy source pixels";
        return false;
    }

    std::ofstream output(outputPath, std::ios::binary);
    if (!output)
    {
        CleanupDib(dibDc, dibBitmap, oldBitmap);
        *error = "could not open batch output file";
        return false;
    }

    WriteBmpHeader(&output, width, height, rowStride);

    std::vector<unsigned char> row(static_cast<std::size_t>(rowStride));
    const unsigned char *pixels = static_cast<const unsigned char *>(bits);
    const int dibStride = width * 4;
    for (int y = height - 1; y >= 0; --y)
    {
        const unsigned char *sourceRow = pixels + y * dibStride;
        std::fill(row.begin(), row.end(), 0);
        for (int x = 0; x < width; ++x)
        {
            const int offset = x * 3;
            const int sourceOffset = x * 4;
            row[static_cast<std::size_t>(offset)] = sourceRow[sourceOffset];
            row[static_cast<std::size_t>(offset + 1)] = sourceRow[sourceOffset + 1];
            row[static_cast<std::size_t>(offset + 2)] = sourceRow[sourceOffset + 2];
        }
        output.write(reinterpret_cast<const char *>(row.data()), row.size());
        if (!output)
        {
            CleanupDib(dibDc, dibBitmap, oldBitmap);
            *error = "could not write batch output file";
            return false;
        }
    }

    CleanupDib(dibDc, dibBitmap, oldBitmap);
    return true;
}

bool WriteBmpFromIntensityPlane(const float *plane, int width, int height, const char *outputPath, std::string *error)
{
    if (plane == 0)
    {
        *error = "missing intensity plane";
        return false;
    }
    if (width <= 0 || height <= 0)
    {
        *error = "invalid image dimensions";
        return false;
    }
    if (outputPath == 0 || *outputPath == '\0')
    {
        *error = "missing output path";
        return false;
    }

    std::ofstream output(outputPath, std::ios::binary);
    if (!output)
    {
        *error = "could not open batch output file";
        return false;
    }

    const int rowBytes = width * 3;
    const int rowStride = (rowBytes + 3) & ~3;
    WriteBmpHeader(&output, width, height, rowStride);

    std::vector<unsigned char> row(static_cast<std::size_t>(rowStride));
    for (int y = height - 1; y >= 0; --y)
    {
        std::fill(row.begin(), row.end(), 0U);
        for (int x = 0; x < width; ++x)
        {
            const unsigned char gray = IntensityToByte(plane[static_cast<std::size_t>(y * width + x)]);
            const int offset = x * 3;
            row[static_cast<std::size_t>(offset)] = gray;
            row[static_cast<std::size_t>(offset + 1)] = gray;
            row[static_cast<std::size_t>(offset + 2)] = gray;
        }
        output.write(reinterpret_cast<const char *>(row.data()), row.size());
        if (!output)
        {
            *error = "could not write batch output file";
            return false;
        }
    }

    return true;
}

} // namespace capow
