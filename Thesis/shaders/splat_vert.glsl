#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aSh;
layout (location = 2) in float aOpacity;
layout (location = 3) in vec3 aScale;
layout (location = 4) in vec4 aQuaternion;

uniform mat4 uView;
uniform mat4 uProj;

out vec3 vSh;
out float vOpacity;

void main()
{
	vSh = aSh;
	vOpacity = aOpacity;

	gl_Position = uProj * uView * vec4(aPos.x,-aPos.y,aPos.z,1.0);
}