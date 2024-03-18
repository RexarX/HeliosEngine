#type vertex
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
  //gl_Position = projection * view * model * vec4(aPos, 1.0);
  gl_Position = vec4(aPos, 1.0);
  TexCoord = aTexCoord;
}

#type fragment
#version 460 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main()
{
  //FragColor = texture(texture1, TexCoord);
  FragColor = vec4(1.0f,1.0f,1.0f,1.0f);
}