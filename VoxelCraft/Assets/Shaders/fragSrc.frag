#version 460 core

in vec3 v_Position;
in vec4 v_Color;

void main()
{
	gl_FragColor = vec4(v_Position * 0.5 + 0.5, 1.0);
	gl_FragColor = v_Color;
}