#version 430 core

in vec3 vSh;
in float vOpacity;
in flat vec2 vCenterPos;
in flat mat2 vICov2D;
in flat float vDepth;
in flat uint vSplatId;

uniform int uDebugMode; //0=RGB 1=Depth 2=Alpha 3=Overdraw 4=Ellipse outline 5=Splat ID
uniform float uNear;
uniform float uFar;

out vec4 fragColor;

vec3 idToColor(uint id)
{
    float r = fract(sin(float(id) * 12.9898) * 43758.5453);
    float g = fract(sin(float(id) * 78.233) * 43758.5453);
    float b = fract(sin(float(id) * 37.719) * 43758.5453);
    return vec3(r, g, b);
}

void main()
{
    vec2 d = gl_FragCoord.xy - vCenterPos;
    float power = -0.5 * (d.x * d.x * vICov2D[0][0] + 2.0 * d.x * d.y * vICov2D[0][1] + d.y * d.y * vICov2D[1][1]);
    if (power > 0.0) discard;
    float alpha = min(0.99, vOpacity * exp(power));

    if (uDebugMode == 4)
    {

        if (alpha < 0.0039 || alpha > 0.05) discard;
        fragColor = vec4(1.0, 1.0, 0.0, 1.0);
        return;
    }

    if (alpha < 0.0039) discard;

    if (uDebugMode == 3)
    {

        fragColor = vec4(vec3(0.08), 1.0);
        return;
    }

    vec3 color;
    if (uDebugMode == 1)
    {
        float depthNorm = clamp((vDepth - uNear) / max(uFar - uNear, 1e-4), 0.0, 1.0);
        color = vec3(depthNorm);
    }
    else if (uDebugMode == 2)
    {
        color = vec3(vOpacity);
    }
    else if (uDebugMode == 5)
    {
        color = idToColor(vSplatId);
    }
    else
    {
        color = max(vSh, 0.0);
    }
    fragColor = vec4(color * alpha, alpha);
}
