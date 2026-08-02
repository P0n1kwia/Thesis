#version 430 core

layout (location = 5) in vec3 aQuadPos;

layout(std430,binding =0) readonly buffer SplatsBuffer{float splatData[];};
layout(std430, binding=1) readonly buffer IndexBuffer{int splatIndex[];};

uniform mat4 uView;
uniform mat4 uProj;
uniform vec2 uScreenSize;
uniform mat4 uModel;
uniform vec3 uCamPos;

out vec3 vSh;
out float vOpacity;
out flat vec2 vCenterPos;
out flat mat2 vICov2D; //symetric so we need a,b,c [a,b, c]

const float SH_C0 = 0.28209479177387814;
const float SH_C1 = 0.4886025119029199;
const uint SPLAT_STRIDE = 23u;
void main()
{
	uint id = splatIndex[gl_InstanceID];
	uint base  = id * SPLAT_STRIDE;
	vec3  aPos        = vec3(splatData[base + 0u], splatData[base + 1u], splatData[base + 2u]);
	vec3  aSh         = vec3(splatData[base + 3u], splatData[base + 4u], splatData[base + 5u]);
	float aOpacity    = splatData[base + 6u];
	vec3  aScale      = vec3(splatData[base + 7u], splatData[base + 8u], splatData[base + 9u]);
	vec4  aQuaternion = vec4(splatData[base + 10u], splatData[base + 11u], splatData[base + 12u], splatData[base + 13u]);

	vec3 c1 =		  vec3(splatData[base + 14u], splatData[base+15u], splatData[base + 16u]);
	vec3 c2 =		  vec3(splatData[base + 17u], splatData[base+18u], splatData[base + 19u]);
	vec3 c3 =		  vec3(splatData[base + 20u], splatData[base+21u], splatData[base + 22u]);

	vec3 worldPos = vec3(uModel * vec4(aPos, 1.0));
	vec3 dir = normalize(worldPos - uCamPos);
	float dx = dir.x, dy = dir.y, dz = dir.z;
	vec3 color = SH_C0 * aSh + SH_C1 * (-dy * c1 + dz * c2 - dx * c3);
	vSh = color + 0.5;
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
	mat4 MV = uView * uModel;
	vec4 t = MV * vec4(aPos,1.0);
	float fx = uProj[0].x * uScreenSize.x * 0.5; 
	float fy = uProj[1].y * uScreenSize.y * 0.5;

	float tz = -t.z;
	if (tz < 0.2) { gl_Position = vec4(0.0, 0.0, 2.0, 1.0); return; }
	float limx = 1.3 * (uScreenSize.x * 0.5) / fx;
	float limy = 1.3 * (uScreenSize.y * 0.5) / fy;
	float tx = clamp(t.x / tz, -limx, limx) * tz;
	float ty = clamp(t.y / tz, -limy, limy) * tz;
	mat3 J = mat3(
		vec3(fx / tz, 0.0,     fx * tx / (tz*tz)),
		vec3(0.0,     fy / tz, fy * ty / (tz*tz)),
		vec3(0.0,     0.0,     0.0)
	);
	mat3 W = mat3(MV);
	mat3 Cov2D = J * W * Cov * transpose(W) * transpose(J);
	mat2 D2 = mat2(
	vec2(Cov2D[0].xy),
	vec2(Cov2D[1].xy)
	);
	D2[0][0] += 0.3;
	D2[1][1] += 0.3;

	const float MAX_SPLAT_RADIUS_PX = 1024.0;
	vec2 extent = min(vec2(
		ceil(3.0 * sqrt(max(0.000001, D2[0][0]))),
		ceil(3.0 * sqrt(max(0.000001, D2[1][1])))
	), vec2(MAX_SPLAT_RADIUS_PX));

	vec4 clipPos = uProj * t;
	vCenterPos = vec2((clipPos.xyz/clipPos.w) * 0.5 + 0.5) * uScreenSize;

	clipPos.xy += aQuadPos.xy * extent * vec2(2.0) / uScreenSize * clipPos.w;


	vICov2D = inverse(D2);

	gl_Position = clipPos;
}