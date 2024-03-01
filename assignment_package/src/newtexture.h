#ifndef NEWTEXTURE_H
#define NEWTEXTURE_H


#include <openglcontext.h>
#include <la.h>
#include <memory>

class NewTexture
{
public:
    NewTexture(OpenGLContext* context);
    ~NewTexture();

    void create(const char *texturePath);
    void load(int texSlot);
    void bind(int texSlot);
    int getHandle();

private:
    OpenGLContext* context;
    GLuint m_textureHandle;
    std::shared_ptr<QImage> m_textureImage;
};

#endif // NEWTEXTURE_H
