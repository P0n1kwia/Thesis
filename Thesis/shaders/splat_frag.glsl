#version 430 core
in vec3 vSh;
in float vOpacity;

out vec4 fragColor;

void main()
{
	fragColor = vec4(vSh,1.0);
}