#version 410 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;
uniform mat4 model, view, projection;
out vec3 worldPosition, worldNormal, albedo;
void main() {
    vec4 p = model * vec4(position, 1);
    worldPosition = p.xyz;
    worldNormal = mat3(model) * normal;
    albedo = color;
    gl_Position = projection * view * p;
}
