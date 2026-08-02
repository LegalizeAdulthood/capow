#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

struct Pixel
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

struct Image
{
    int width;
    int height;
    std::vector<Pixel> pixels;
};

struct Options
{
    std::string expectedPath;
    std::string actualPath;
    std::string diffPath;
};

void PrintUsage()
{
    std::cerr << "usage: image-compare --expected expected.bmp --actual actual.bmp --diff diff.bmp\n";
}

bool ReadFile(const std::string &path, std::vector<unsigned char> *bytes, std::string *error)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
    {
        *error = "could not open " + path;
        return false;
    }

    bytes->assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    if (input.bad())
    {
        *error = "could not read " + path;
        return false;
    }
    return true;
}

bool ReadU16(const std::vector<unsigned char> &bytes, std::size_t offset, unsigned int *value)
{
    if (offset + 2U > bytes.size())
    {
        return false;
    }
    *value = static_cast<unsigned int>(bytes[offset]) | (static_cast<unsigned int>(bytes[offset + 1U]) << 8U);
    return true;
}

bool ReadU32(const std::vector<unsigned char> &bytes, std::size_t offset, unsigned int *value)
{
    if (offset + 4U > bytes.size())
    {
        return false;
    }
    *value = static_cast<unsigned int>(bytes[offset]) | (static_cast<unsigned int>(bytes[offset + 1U]) << 8U) |
        (static_cast<unsigned int>(bytes[offset + 2U]) << 16U) | (static_cast<unsigned int>(bytes[offset + 3U]) << 24U);
    return true;
}

int ToSignedInt(unsigned int value)
{
    return static_cast<int>(static_cast<std::int32_t>(value));
}

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

std::size_t PixelIndex(const Image &image, int x, int y)
{
    return static_cast<std::size_t>(y * image.width + x);
}

bool ReadBmp(const std::string &path, Image *image, std::string *error)
{
    std::vector<unsigned char> bytes;
    if (!ReadFile(path, &bytes, error))
    {
        return false;
    }
    if (bytes.size() < 54U || bytes[0] != 'B' || bytes[1] != 'M')
    {
        *error = path + " is not a BMP file";
        return false;
    }

    unsigned int pixelOffset = 0U;
    unsigned int dibHeaderSize = 0U;
    unsigned int rawWidth = 0U;
    unsigned int rawHeight = 0U;
    unsigned int planes = 0U;
    unsigned int bitCount = 0U;
    unsigned int compression = 0U;
    if (!ReadU32(bytes, 10U, &pixelOffset) || !ReadU32(bytes, 14U, &dibHeaderSize) || !ReadU32(bytes, 18U, &rawWidth) ||
        !ReadU32(bytes, 22U, &rawHeight) || !ReadU16(bytes, 26U, &planes) || !ReadU16(bytes, 28U, &bitCount) ||
        !ReadU32(bytes, 30U, &compression))
    {
        *error = path + " has a truncated BMP header";
        return false;
    }
    if (dibHeaderSize < 40U || planes != 1U || compression != 0U || (bitCount != 24U && bitCount != 32U))
    {
        *error = path + " must be an uncompressed 24-bit or 32-bit BMP";
        return false;
    }

    const int width = ToSignedInt(rawWidth);
    const int signedHeight = ToSignedInt(rawHeight);
    const bool topDown = signedHeight < 0;
    const int height = topDown ? -signedHeight : signedHeight;
    if (width <= 0 || height <= 0)
    {
        *error = path + " has invalid BMP dimensions";
        return false;
    }

    const int bytesPerPixel = static_cast<int>(bitCount / 8U);
    const std::size_t rowStride = static_cast<std::size_t>(((width * static_cast<int>(bitCount)) + 31) / 32 * 4);
    const std::size_t requiredSize =
        static_cast<std::size_t>(pixelOffset) + rowStride * static_cast<std::size_t>(height);
    if (requiredSize > bytes.size())
    {
        *error = path + " has truncated pixel data";
        return false;
    }

    image->width = width;
    image->height = height;
    image->pixels.assign(static_cast<std::size_t>(width * height), Pixel{0U, 0U, 0U});

    for (int y = 0; y < height; ++y)
    {
        const int fileY = topDown ? y : height - 1 - y;
        const std::size_t rowOffset =
            static_cast<std::size_t>(pixelOffset) + rowStride * static_cast<std::size_t>(fileY);
        for (int x = 0; x < width; ++x)
        {
            const std::size_t sourceOffset = rowOffset + static_cast<std::size_t>(x * bytesPerPixel);
            Pixel pixel;
            pixel.blue = bytes[sourceOffset];
            pixel.green = bytes[sourceOffset + 1U];
            pixel.red = bytes[sourceOffset + 2U];
            image->pixels[PixelIndex(*image, x, y)] = pixel;
        }
    }
    return true;
}

bool WriteBmp(const std::string &path, const Image &image, std::string *error)
{
    if (image.width <= 0 || image.height <= 0 ||
        image.pixels.size() != static_cast<std::size_t>(image.width * image.height))
    {
        *error = "invalid image data for " + path;
        return false;
    }

    std::ofstream output(path.c_str(), std::ios::binary);
    if (!output)
    {
        *error = "could not open " + path;
        return false;
    }

    const int rowBytes = image.width * 3;
    const int rowStride = (rowBytes + 3) & ~3;
    const unsigned int pixelOffset = 54U;
    const unsigned int imageSize = static_cast<unsigned int>(rowStride * image.height);
    const unsigned int fileSize = pixelOffset + imageSize;

    output.put('B');
    output.put('M');
    WriteU32(&output, fileSize);
    WriteU16(&output, 0U);
    WriteU16(&output, 0U);
    WriteU32(&output, pixelOffset);
    WriteU32(&output, 40U);
    WriteU32(&output, static_cast<unsigned int>(image.width));
    WriteU32(&output, static_cast<unsigned int>(image.height));
    WriteU16(&output, 1U);
    WriteU16(&output, 24U);
    WriteU32(&output, 0U);
    WriteU32(&output, imageSize);
    WriteU32(&output, 0U);
    WriteU32(&output, 0U);
    WriteU32(&output, 0U);
    WriteU32(&output, 0U);

    std::vector<unsigned char> row(static_cast<std::size_t>(rowStride), 0U);
    for (int y = image.height - 1; y >= 0; --y)
    {
        std::fill(row.begin(), row.end(), 0U);
        for (int x = 0; x < image.width; ++x)
        {
            const Pixel &pixel = image.pixels[PixelIndex(image, x, y)];
            const std::size_t offset = static_cast<std::size_t>(x * 3);
            row[offset] = pixel.blue;
            row[offset + 1U] = pixel.green;
            row[offset + 2U] = pixel.red;
        }
        output.write(reinterpret_cast<const char *>(row.data()), row.size());
        if (!output)
        {
            *error = "could not write " + path;
            return false;
        }
    }

    return true;
}

Pixel MakePixel(unsigned char red, unsigned char green, unsigned char blue)
{
    Pixel pixel;
    pixel.red = red;
    pixel.green = green;
    pixel.blue = blue;
    return pixel;
}

bool SamePixel(const Pixel &left, const Pixel &right)
{
    return left.red == right.red && left.green == right.green && left.blue == right.blue;
}

int AbsDiff(unsigned char left, unsigned char right)
{
    return std::abs(static_cast<int>(left) - static_cast<int>(right));
}

Pixel DiffPixel(const Pixel &expected, const Pixel &actual)
{
    const int redDiff = AbsDiff(expected.red, actual.red);
    const int greenDiff = AbsDiff(expected.green, actual.green);
    const int blueDiff = AbsDiff(expected.blue, actual.blue);
    const int maxDiff = std::max(redDiff, std::max(greenDiff, blueDiff));
    if (maxDiff == 0)
    {
        return MakePixel(0U, 0U, 0U);
    }
    const unsigned char visibleDiff = static_cast<unsigned char>(std::max(64, maxDiff));
    return MakePixel(visibleDiff, 0U, 0U);
}

bool TryGetPixel(const Image &image, int x, int y, Pixel *pixel)
{
    if (x < 0 || y < 0 || x >= image.width || y >= image.height)
    {
        return false;
    }
    *pixel = image.pixels[PixelIndex(image, x, y)];
    return true;
}

Image MakeDiffImage(const Image &expected, const Image &actual)
{
    Image diff;
    diff.width = std::max(expected.width, actual.width);
    diff.height = std::max(expected.height, actual.height);
    diff.pixels.assign(static_cast<std::size_t>(diff.width * diff.height), MakePixel(0U, 0U, 0U));

    for (int y = 0; y < diff.height; ++y)
    {
        for (int x = 0; x < diff.width; ++x)
        {
            Pixel expectedPixel;
            Pixel actualPixel;
            const bool hasExpected = TryGetPixel(expected, x, y, &expectedPixel);
            const bool hasActual = TryGetPixel(actual, x, y, &actualPixel);
            Pixel diffPixel = MakePixel(0U, 0U, 0U);
            if (hasExpected && hasActual)
            {
                diffPixel = DiffPixel(expectedPixel, actualPixel);
            }
            else if (hasExpected)
            {
                diffPixel = MakePixel(0U, 0U, 255U);
            }
            else if (hasActual)
            {
                diffPixel = MakePixel(0U, 255U, 0U);
            }
            diff.pixels[PixelIndex(diff, x, y)] = diffPixel;
        }
    }

    return diff;
}

bool ImagesEqual(const Image &expected, const Image &actual, int *differentPixels)
{
    *differentPixels = 0;
    if (expected.width != actual.width || expected.height != actual.height)
    {
        return false;
    }

    for (std::size_t i = 0; i < expected.pixels.size(); ++i)
    {
        if (!SamePixel(expected.pixels[i], actual.pixels[i]))
        {
            ++*differentPixels;
        }
    }
    return *differentPixels == 0;
}

bool NeedValue(int index, int argc, const char *name, std::string *error)
{
    if (index + 1 < argc)
    {
        return true;
    }
    *error = "missing value for ";
    *error += name;
    return false;
}

bool ParseArguments(int argc, const char *argv[], Options *options, std::string *error)
{
    if (argc == 1)
    {
        *error = "missing arguments";
        return false;
    }

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--expected")
        {
            if (!NeedValue(i, argc, "--expected", error))
            {
                return false;
            }
            options->expectedPath = argv[++i];
        }
        else if (arg == "--actual")
        {
            if (!NeedValue(i, argc, "--actual", error))
            {
                return false;
            }
            options->actualPath = argv[++i];
        }
        else if (arg == "--diff")
        {
            if (!NeedValue(i, argc, "--diff", error))
            {
                return false;
            }
            options->diffPath = argv[++i];
        }
        else if (arg == "--help")
        {
            PrintUsage();
            std::exit(0);
        }
        else
        {
            *error = "unknown option " + arg;
            return false;
        }
    }

    if (options->expectedPath.empty() || options->actualPath.empty() || options->diffPath.empty())
    {
        *error = "missing --expected, --actual, or --diff";
        return false;
    }
    return true;
}

int CompareImages(const Options &options)
{
    std::string error;
    Image expected;
    Image actual;
    if (!ReadBmp(options.expectedPath, &expected, &error) || !ReadBmp(options.actualPath, &actual, &error))
    {
        std::cerr << error << "\n";
        return 2;
    }

    int differentPixels = 0;
    if (ImagesEqual(expected, actual, &differentPixels))
    {
        return 0;
    }

    const Image diff = MakeDiffImage(expected, actual);
    if (!WriteBmp(options.diffPath, diff, &error))
    {
        std::cerr << error << "\n";
        return 2;
    }

    if (expected.width != actual.width || expected.height != actual.height)
    {
        std::cerr << "image dimensions differ: expected " << expected.width << "x" << expected.height << ", actual "
                  << actual.width << "x" << actual.height << "\n";
    }
    else
    {
        std::cerr << "images differ: " << differentPixels << " pixels differ\n";
    }
    std::cerr << "diff written to " << options.diffPath << "\n";
    return 1;
}

} // namespace

int main(int argc, const char *argv[])
{
    Options options;
    std::string error;
    if (!ParseArguments(argc, argv, &options, &error))
    {
        std::cerr << error << "\n";
        PrintUsage();
        return 2;
    }

    return CompareImages(options);
}
