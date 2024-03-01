#version 330

in vec2 fs_UV;

out vec4 color;

uniform sampler2D u_RenderedTexture;

uniform ivec2 u_Dimensions;

void main()
{
    //color = texture(u_RenderedTexture, fs_UV * u_Dimensions);
    color = texture(u_RenderedTexture, fs_UV);
}
