#type vertex
#version 460 core
layout (location = 0) in vec3 a_Pos;

out vec3 TexCoords;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    TexCoords = a_Pos;
    gl_Position = u_Projection * u_View * vec4(a_Pos, 1.0);
}

#type fragment
#version 460 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{
  FragColor = texture(skybox, TexCoords);
}