#version 460

layout (location = 0) in vec3 a_Pos;

layout (location = 0) out vec3 outColor;

void main() 
{
	const vec3 colors[3] = vec3[3](
		vec3(1.0f, 0.0f, 0.0f),
		vec3(0.0f, 1.0f, 0.0f),
		vec3(0.0f, 0.0f, 1.0f)
	);

	const vec3 positions[3] = vec3[3](
		vec3(1.0f, 1.0f, 0.0f),
		vec3(-1.0f, 1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f)
	);

	gl_Position = vec4(a_Pos, 1.0f); //u_ProjectionView * u_Transform * vec4(a_Pos, 1.0);
	outColor = colors[gl_VertexIndex];
}