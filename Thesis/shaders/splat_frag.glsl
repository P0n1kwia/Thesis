#version 430 core

in vec3 vSh;
in float vOpacity;
in flat vec2 vCenterPos;
in flat mat2 vICov2D;

out vec4 fragColor;

void main()
{
    vec2 d = gl_FragCoord.xy - vCenterPos;
    float power = -0.5 * (d.x * d.x * vICov2D[0][0] + 2.0 * d.x * d.y * vICov2D[0][1] + d.y * d.y * vICov2D[1][1]);  
    if (power > 0.0) discard;
    float alpha = min(0.99, vOpacity * exp(power));
    if (alpha < 0.0039) discard;
    vec3 color = max(vSh, 0.0);
    fragColor = vec4(color * alpha, alpha);
}