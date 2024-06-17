#version 460

layout (location = 0) in vec3 a_Pos;

layout (binding = 0) uniform SceneData
{
	mat4 u_ProjectionView;
	mat4 u_Transform;
};

layout (location = 0) out vec3 outColor;

void main() 
{
	const vec3 colors[3] = vec3[3](
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f)
	);

	gl_Position = u_ProjectionView * u_Transform * vec4(a_Pos, 1.0f);
	outColor = colors[gl_VertexIndex];
}