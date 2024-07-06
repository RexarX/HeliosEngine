#version 460

layout (location = 0) in vec2 TexCoord;

layout(binding = 1) uniform sampler2D inTexture;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(inTexture, TexCoord);
}