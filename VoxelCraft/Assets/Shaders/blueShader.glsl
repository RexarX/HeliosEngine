#type fragment

#version 460 core
			
in vec3 v_Position;

void main()
{
	gl_FragColor = vec4(0.2, 0.3, 0.8, 1.0);
}

#type vertex

#version 460 core
			
layout(location = 0) in vec3 a_Position;

out vec3 v_Position;

void main()
{
	v_Position = a_Position;
	gl_Position = vec4(a_Position, 1.0);	
}