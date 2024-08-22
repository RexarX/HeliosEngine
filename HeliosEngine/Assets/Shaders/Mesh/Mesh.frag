#version 460

layout (location = 0) in vec2 a_TexCoord;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = vec4(a_TexCoord, 1.0f, 1.0f);
}