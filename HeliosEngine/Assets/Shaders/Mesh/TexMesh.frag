#include "Mesh.frag"

layout (location = 0) in vec2 a_TexCoords;

layout (set = 3, binding = 0) uniform sampler2D inTexture;

layout (location = 0) out vec4 outFragColor;

void main()
{
	outFragColor = texture(inTexture, a_TexCoords);
}