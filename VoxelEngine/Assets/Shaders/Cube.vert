#version 460

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TexCoord;

layout (binding = 0) uniform SceneData
{
	mat4 u_View;
	mat4 u_Projection;
	mat4 u_ProjectionView;
	mat4 u_Transform;
};

layout (location = 0) out vec3 outColor;

void main() 
{
	gl_Position = u_ProjectionView * u_Transform * vec4(a_Pos, 1.0f);
	outColor = vec3(a_TexCoord, 1.0f);
}