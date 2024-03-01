#version 330

in vec2 fs_UV;

out vec4 out_Color;

uniform sampler2D u_Texture;

#define PLUS_LENGTH 0.1f
#define PLUS_WIDTH 0.025f

void main()
{
    out_Color = texture(u_Texture, fs_UV);
//    out_Color = vec4(fs_UV.x,fs_UV.y,0,1);
}
