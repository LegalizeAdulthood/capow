#include "OpenGlImagePresenter.hpp"

#include "Types.h"

#include <GL/gl.h>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

namespace capow
{

OpenGlImagePresenter::OpenGlImagePresenter() :
    texture(0U),
    textureWidth(0),
    textureHeight(0)
{
}

OpenGlImagePresenter::~OpenGlImagePresenter()
{
    Release();
}

bool OpenGlImagePresenter::Present(const ImageBuffer &image, int left, int top, int width, int height)
{
    if (image.Data() == 0 || image.Width() <= 0 || image.Height() <= 0 || width <= 0 || height <= 0)
        return false;
    if (wglGetCurrentContext() == NULL)
        return false;
    if (!EnsureTexture(image.Width(), image.Height()))
        return false;

    Upload(image);
    DrawQuad(left, top, width, height);
    return true;
}

void OpenGlImagePresenter::DrawLine(int x0, int y0, int x1, int y1, COLORREF color, float lineWidth)
{
    if (wglGetCurrentContext() == NULL || lineWidth <= 0.0F)
        return;

    GLint oldMatrixMode;
    BeginScreenDrawing(&oldMatrixMode);
    SetColor(color);
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    glVertex2i(x0, y0);
    glVertex2i(x1, y1);
    glEnd();
    EndScreenDrawing(oldMatrixMode);
}

void OpenGlImagePresenter::DrawColoredLines(
    const POINT *starts, const POINT *ends, const COLORREF *colors, int count, float lineWidth)
{
    if (wglGetCurrentContext() == NULL || starts == 0 || ends == 0 || colors == 0 || count <= 0 || lineWidth <= 0.0F)
        return;

    GLint oldMatrixMode;
    BeginScreenDrawing(&oldMatrixMode);
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
    for (int i = 0; i < count; ++i)
    {
        SetColor(colors[i]);
        glVertex2i(starts[i].x, starts[i].y);
        glVertex2i(ends[i].x, ends[i].y);
    }
    glEnd();
    EndScreenDrawing(oldMatrixMode);
}

void OpenGlImagePresenter::DrawPoints(const POINT *points, int count, COLORREF color, float pointSize)
{
    if (wglGetCurrentContext() == NULL || points == 0 || count <= 0 || pointSize <= 0.0F)
        return;

    GLint oldMatrixMode;
    BeginScreenDrawing(&oldMatrixMode);
    SetColor(color);
    glPointSize(pointSize);
    glBegin(GL_POINTS);
    for (int i = 0; i < count; ++i)
        glVertex2i(points[i].x, points[i].y);
    glEnd();
    EndScreenDrawing(oldMatrixMode);
}

void OpenGlImagePresenter::DrawRectangle(int left, int top, int right, int bottom, COLORREF color, float lineWidth)
{
    if (wglGetCurrentContext() == NULL || lineWidth <= 0.0F)
        return;

    GLint oldMatrixMode;
    BeginScreenDrawing(&oldMatrixMode);
    SetColor(color);
    glLineWidth(lineWidth);
    glBegin(GL_LINE_LOOP);
    glVertex2i(left, top);
    glVertex2i(right, top);
    glVertex2i(right, bottom);
    glVertex2i(left, bottom);
    glEnd();
    EndScreenDrawing(oldMatrixMode);
}

void OpenGlImagePresenter::Release()
{
    if (texture == 0U || wglGetCurrentContext() == NULL)
        return;

    glDeleteTextures(1, &texture);
    texture = 0U;
    textureWidth = 0;
    textureHeight = 0;
}

bool OpenGlImagePresenter::EnsureTexture(int width, int height)
{
    GLint oldTextureBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureBinding);

    if (texture == 0U)
    {
        glGenTextures(1, &texture);
        if (texture == 0U)
            return false;

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }

    if (textureWidth == width && textureHeight == height)
    {
        glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(oldTextureBinding));
        return true;
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(oldTextureBinding));
    textureWidth = width;
    textureHeight = height;
    return true;
}

void OpenGlImagePresenter::Upload(const ImageBuffer &image)
{
    GLint oldUnpackAlignment;
    GLint oldTextureBinding;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &oldUnpackAlignment);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTextureBinding);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.Width(), image.Height(), GL_BGRA, GL_UNSIGNED_BYTE, image.Data());
    glBindTexture(GL_TEXTURE_2D, static_cast<unsigned int>(oldTextureBinding));
    glPixelStorei(GL_UNPACK_ALIGNMENT, oldUnpackAlignment);
}

void OpenGlImagePresenter::DrawQuad(int left, int top, int width, int height)
{
    GLint viewport[4];
    GLint oldMatrixMode;
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_MATRIX_MODE, &oldMatrixMode);

    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texture);
    glColor3f(1.0F, 1.0F, 1.0F);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, viewport[2], viewport[3], 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
    glTexCoord2f(0.0F, 0.0F);
    glVertex2i(left, top);
    glTexCoord2f(1.0F, 0.0F);
    glVertex2i(left + width, top);
    glTexCoord2f(1.0F, 1.0F);
    glVertex2i(left + width, top + height);
    glTexCoord2f(0.0F, 1.0F);
    glVertex2i(left, top + height);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(oldMatrixMode);
    glPopAttrib();
}

void OpenGlImagePresenter::BeginScreenDrawing(GLint *oldMatrixMode)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetIntegerv(GL_MATRIX_MODE, oldMatrixMode);

    glPushAttrib(
        GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT | GL_LINE_BIT | GL_POINT_BIT |
        GL_TEXTURE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, viewport[2], viewport[3], 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void OpenGlImagePresenter::EndScreenDrawing(GLint oldMatrixMode)
{
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(oldMatrixMode);
    glPopAttrib();
}

void OpenGlImagePresenter::SetColor(COLORREF color)
{
    glColor3ub(GetRValue(color), GetGValue(color), GetBValue(color));
}

} // namespace capow
