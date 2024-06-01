#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec2 a_TexCoord;
layout (location = 2) in mat4 a_Transform;

out vec2 TexCoord;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
  gl_Position = u_Projection * u_View * a_Transform * vec4(a_Pos, 1.0);
  TexCoord = a_TexCoord;
}