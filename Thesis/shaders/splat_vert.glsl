#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aSh;
layout (location = 2) in float aOpacity;
layout (location = 3) in vec3 aScale;
layout (location = 4) in vec4 aQuaternion;
layout (location = 5) in vec3 aQuadPos;

uniform mat4 uView;
uniform mat4 uProj;
uniform vec2 uScreenSize;

out vec3 vSh;
out float vOpacity;
out flat vec2 vCenterPos;
out flat mat2 vICov2D; //symetric so we need a,b,c [a,b, c]



void main()
{
	vSh = aSh;
	vOpacity = aOpacity;

	mat3 S = mat3(
    vec3(aScale.x, 0.0, 0.0),
    vec3(0.0, aScale.y, 0.0),
    vec3(0.0, 0.0, aScale.z)
	);
	float w = aQuaternion.x;
	float x = aQuaternion.y;
	float y = aQuaternion.z;
	float z = aQuaternion.w;
	mat3 R = mat3(
	vec3(1-2*(y*y+z*z), 2*(x*y - w*z), 2*(x*z + w*y)),
	vec3(2*(x*y + w*z), 1-2*(x*x+z*z), 2*(z*y - w*x)),
	vec3(2*(x*z - w*y), 2*(y*z + w*x), 1-2*(x*x+y*y))
	);

	mat3 M = R * S;
	mat3 Mt = transpose(M);
	mat3 Cov = M*Mt;
	vec4 t = uView * vec4(aPos.x,-aPos.y,aPos.z, 1.0);
	float fx = uProj[0].x * uScreenSize.x * 0.5; 
	float fy = uProj[1].y * uScreenSize.y * 0.5;
	mat3 J = mat3(
    vec3(fx / t.z, 0, -fx * t.x / (t.z*t.z)),
    vec3(0.0, fy / t.z, -fy * t.y / (t.z*t.z)),
    vec3(0.0, 0.0, 0.0)
);
	mat3 W = mat3(uView);
	mat3 Cov2D = J * W * Cov * transpose(W) * transpose(J);
	mat2 D2 = mat2(
	vec2(Cov2D[0].xy),
	vec2(Cov2D[1].xy)
	);
	D2[0][0] += 0.3;
	D2[1][1] += 0.3;

	vec2 extent = vec2(
		ceil(3.0 * sqrt(max(0.000001, D2[0][0]))),
		ceil(3.0 * sqrt(max(0.000001, D2[1][1])))
	);
	
	vec4 clipPos = uProj * t;
	vCenterPos = vec2((clipPos.xyz/clipPos.w) * 0.5 + 0.5) * uScreenSize;

	clipPos.xy += aQuadPos.xy * extent * vec2(2.0) / uScreenSize * clipPos.w;


	vICov2D = inverse(D2);

	gl_Position = clipPos;
}