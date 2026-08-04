#version 430 core

layout(location = 5) in vec3 aQuadPos;      

layout(std430, binding = 1) readonly buffer IndexBuffer   { uint splatIndex[]; };
layout(std430, binding = 2) readonly buffer PreprocBuffer { vec4 preprocBuffer[]; };

uniform vec2 uScreenSize;

out vec3 vSh;
out float vOpacity;
out flat vec2 vCenterPos;
out flat mat2 vICov2D;
out flat float vDepth;
out flat uint vSplatId;

void main()
{
    uint id = splatIndex[gl_InstanceID];

    vec4 p0 = preprocBuffer[3u*id + 0u];
    vec4 p1 = preprocBuffer[3u*id + 1u];
    vec4 p2 = preprocBuffer[3u*id + 2u];

    vec2 center = p0.xy;
    vec2 extent = p0.zw;

    if (extent.x <= 0.0) { gl_Position = vec4(0.0, 0.0, 2.0, 1.0); return; }

    vCenterPos = center;
    vICov2D    = mat2(p1.x, p1.y, p1.y, p1.z);
    vSh        = p2.rgb;
    vOpacity   = p2.a;
    vDepth     = p1.w;
    vSplatId   = id;

    vec2 px  = center + aQuadPos.xy * extent;
    vec2 ndc = px / uScreenSize * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
}