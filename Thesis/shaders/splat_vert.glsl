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
	mat3 J = mat3(
	vec3(uProj[0].x /t.z, 0, -uProj[0].x * t.x / (t.z*t.z)),
	vec3(0.0,uProj[1].y/t.z, -uProj[1].y * t.z /(t.z*t.z)),
	vec3(0.0,0.0,0.0)
	);
	mat3 W = mat3(uView);
	mat3 Cov2D = J * W * Cov * transpose(W) * transpose(J);
	mat2 D2 = mat2(
	vec2(Cov2D[0].xy),
	vec2(Cov2D[1].xy)
	);
	float mid = 0.5 * (D2[0][0] + D2[1][1]);
	float disc = sqrt(max(0.1,mid*mid - (D2[0][0] * D2[1][1] - D2[0][1]*D2[0][1])));
	float lambda1 = mid + disc;
	float lambda2 = mid - disc;
	float radius = ceil(3.0 * sqrt(max(lambda1,lambda2)));
	
	vec4 clipPos = uProj * t ;
	clipPos.xy += aQuadPos.xy * radius / uScreenSize * clipPos.w;
	gl_Position = clipPos;
}