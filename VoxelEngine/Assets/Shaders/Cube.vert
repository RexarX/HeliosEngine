#version 460

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TexCoord;

layout (binding = 0) uniform SceneData
{
	mat4 projectionView;
	mat4 transform;
} u_SceneData;

layout (location = 0) out vec3 outColor;

void main() 
{
	gl_Position = u_SceneData.projectionView * u_SceneData.transform * vec4(a_Pos, 1.0f);
	outColor = vec3(a_TexCoord, 1.0f);
}