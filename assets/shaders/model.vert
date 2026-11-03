#version 410 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec4 color;
layout(location = 4) in vec2 secondaryTexcoord;
uniform mat4 model, view, projection;
uniform bool grassBlade;
uniform float worldTime;
uniform vec3 cameraPosition;
out vec3 worldNormal;
out vec2 uv;
out vec2 leafUV;
out vec3 worldPosition;
out vec4 vertexColor;
void main() {
    vec4 world = model * vec4(position, 1.0);
    if (grassBlade) {
        float growth = 1.0 - smoothstep(18.0, 28.0, length(world.xz - cameraPosition.xz));
        world.y = .006 + (world.y - .006) * growth;
        float sway =
            .035 * sin(worldTime * 1.6 + world.x * .7 + world.z * .45) * texcoord.y * texcoord.y;
        world.xz += vec2(sway, sway * .4);
    }
    worldNormal = transpose(inverse(mat3(model))) * normal;
    uv = texcoord;
    leafUV = secondaryTexcoord;
    worldPosition = world.xyz;
    vertexColor = color;
    gl_Position = projection * view * world;
}
