#version 460

#ifndef MESH_VERT
#define MESH_VERT

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;

layout (set = 0, binding = 0) readonly buffer ModelMatrices
{
	mat4 modelMatrices[];
}

layout (push_constant) uniform SceneData
{
	mat4 projectionViewMatrix;
}

layout (location = 0) out vec2 a_TexCoord;

void main() 
{	
	gl_Position = projectionViewMatrix * modelMatrices[gl_InstanceID] * vec4(a_Position, 1.0f);
}

#endif