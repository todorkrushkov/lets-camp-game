#version 410 core
in vec3 worldNormal;
in vec2 uv;
in vec2 leafUV;
in vec3 worldPosition;
in vec4 vertexColor;
out vec4 FragColor;
uniform sampler2D baseTexture, emissiveTexture;
uniform bool hasTexture, hasEmissive, unlit, proceduralLeaf, grassBlade;
uniform vec4 baseColor;
uniform vec3 emissiveFactor;
uniform vec3 cameraPosition;
uniform mat3 baseUV, emissiveUV;
uniform int alphaMode;
uniform float alphaCutoff;
void main() {
    if (grassBlade && length(worldPosition.xz - cameraPosition.xz) >= 28.0)
        discard;
    vec4 color = baseColor * vertexColor;
    if (hasTexture)
        color *= texture(baseTexture, (baseUV * vec3(uv, 1.0)).xy);
    if (proceduralLeaf) {
        // Recreate the unbaked Blender leaf shader using its second UV set.
        float along = clamp(leafUV.y, 0.0, 1.0);
        float across = leafUV.x - 0.5;
        float width = 0.47 * pow(max(sin(along * 3.14159265), 0.0), 0.8);
        if (abs(across) > width || width < 0.002)
            discard;
        float variation = 0.5 + 0.5 * sin(dot(worldPosition, vec3(1.37, 2.11, 0.83)));
        vec3 green = mix(vec3(0.035, 0.16, 0.012), vec3(0.10, 0.30, 0.025), variation);
        float middleVein = 1.0 - smoothstep(0.005, 0.018, abs(across));
        float sideVeins = pow(max(0.0, cos((along - abs(across) * 0.7) * 75.0)), 12.0);
        green *= 0.88 + 0.12 * (1.0 - abs(across) / max(width, 0.001));
        color.rgb = green * (1.0 + 0.16 * middleVein + 0.06 * sideVeins);
        color.a = 1.0;
    }
    if (alphaMode == 1 && color.a < alphaCutoff)
        discard;
    vec3 n = normalize(worldNormal);
    if (!gl_FrontFacing)
        n = -n;
    float diffuse = max(dot(n, normalize(vec3(-0.4, 0.8, 0.5))), 0.0);
    float light = unlit ? 1.0 : (grassBlade ? 0.65 + 0.35 * diffuse : 0.4 + 0.6 * diffuse);
    vec3 emission = emissiveFactor;
    if (hasEmissive)
        emission *= texture(emissiveTexture, (emissiveUV * vec3(uv, 1.0)).xy).rgb;
    // Existing sprite renderer outputs display-space colors without framebuffer sRGB.
    vec3 linearColor = color.rgb * light + emission;
    FragColor =
        vec4(pow(max(linearColor, vec3(0.0)), vec3(1.0 / 2.2)), alphaMode == 2 ? color.a : 1.0);
}
