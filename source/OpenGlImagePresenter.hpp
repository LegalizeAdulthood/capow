#ifndef OPENGLIMAGEPRESENTER_HPP
#define OPENGLIMAGEPRESENTER_HPP

#include "ImageBuffer.hpp"
#include "Types.h"

#include <GL/gl.h>

namespace capow
{

class OpenGlImagePresenter
{
public:
    OpenGlImagePresenter();
    ~OpenGlImagePresenter();

    OpenGlImagePresenter(const OpenGlImagePresenter &) = delete;
    OpenGlImagePresenter &operator=(const OpenGlImagePresenter &) = delete;

    bool Present(const ImageBuffer &image, int left, int top, int width, int height);
    void DrawLine(int x0, int y0, int x1, int y1, COLORREF color, float lineWidth = 1.0F);
    void DrawRectangle(int left, int top, int right, int bottom, COLORREF color, float lineWidth = 1.0F);
    void Release();

private:
    bool EnsureTexture(int width, int height);
    void Upload(const ImageBuffer &image);
    void DrawQuad(int left, int top, int width, int height);
    void BeginScreenDrawing(GLint *oldMatrixMode);
    void EndScreenDrawing(GLint oldMatrixMode);
    void SetColor(COLORREF color);

    unsigned int texture;
    int textureWidth;
    int textureHeight;
};

} // namespace capow

#endif
