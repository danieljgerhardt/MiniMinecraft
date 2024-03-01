#pragma once

#include "drawable.h"
#include <la.h>

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Quad : public Drawable {
public:
    Quad(OpenGLContext* context);
    void create();

    ~Quad() override;

    void createVBOdata() override; // To be implemented by subclasses. Populates the VBOs of the Drawable.

    // Getter functions for various GL data
    GLenum drawMode() override;
};
