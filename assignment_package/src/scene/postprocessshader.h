#pragma once

#include "shaderprogram.h"

class PostProcessShader : public ShaderProgram
{
public:

    int attrPos; // A handle for the "in" vec4 representing vertex position in the vertex shader
    int attrUV; // A handle for the "in" vec2 representing the UV coordinates in the vertex shader

    int unifDimensions; // A handle to the "uniform" ivec2 that stores the width and height of the texture being rendered
    int unifSampler2D; // A handle to the "uniform" sampler2D that will be used to read the texture containing the scene render
    int unifTime; // A handle for the "uniform" float representing time in the shader

public:
    PostProcessShader(OpenGLContext* context);
    virtual ~PostProcessShader();

    // Sets up shader-specific handles
    void setupMemberVars();
    // Draw the given object to our screen using this ShaderProgram's shaders
    void draw(Drawable &d, int textureSlot);

    void setDimensions(glm::ivec2 dims);
};
