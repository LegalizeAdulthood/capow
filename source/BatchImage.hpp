#ifndef BATCHIMAGE_HPP
#define BATCHIMAGE_HPP

#include <Windows.h>

#include <string>

namespace capow
{

bool WriteBmpFromHdc(HDC hdc, int left, int top, int width, int height, const char *outputPath, std::string *error);

} // namespace capow

#endif
