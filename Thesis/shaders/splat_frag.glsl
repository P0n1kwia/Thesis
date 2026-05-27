#version 430 core
in vec3 vSh;
in float vOpacity;

out vec4 fragColor;

void main()
{
	
	vec2 coord = gl_PointCoord - vec2(0.5,0.5);
	/*if(dot(coord,coord) > 0.25)
	{
	discard;
	}*/
	fragColor = vec4(vSh,vOpacity);
}