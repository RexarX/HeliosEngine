#type vertex
#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;

out vec2 TexCoord;

uniform mat4 u_Transform;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
  gl_Position = u_Projection * u_View * u_Transform * vec4(a_Pos, 1.0);
  TexCoord = a_TexCoord;
}

#type fragment
#version 460 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
  FragColor = texture(texture1, TexCoord);
  //FragColor = vec4(0.0f,1.0f,0.0f,1.0f);
}