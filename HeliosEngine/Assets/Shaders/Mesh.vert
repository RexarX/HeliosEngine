#version 460

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;

layout (binding = 0) uniform SceneData
{
	mat4 projectionView;
	mat4 transform;
} u_SceneData;

layout (location = 0) out vec2 TexCoord;

void main() 
{
	vec3 colors[3] = {
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f)
	};

	gl_Position = u_SceneData.projectionView * u_SceneData.transform * vec4(a_Position, 1.0f);
	TexCoord = a_TexCoord;
}