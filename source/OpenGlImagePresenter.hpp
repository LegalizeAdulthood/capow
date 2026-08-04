#ifndef OPENGLIMAGEPRESENTER_HPP
#define OPENGLIMAGEPRESENTER_HPP

#include "ImageBuffer.hpp"

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
    void Release();

private:
    bool EnsureTexture(int width, int height);
    void Upload(const ImageBuffer &image);
    void DrawQuad(int left, int top, int width, int height);

    unsigned int texture;
    int textureWidth;
    int textureHeight;
};

} // namespace capow

#endif
