#version 410 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 uvOffset;
uniform vec2 uvScale;
void main() {
    // Keep world-up vertical; only turn the front facade around the Y axis.
    // Source assets contain front views only, so use cylindrical billboards.
    vec3 right = vec3(view[0][0], 0.0, view[2][0]);
    if (dot(right, right) < 0.0001)
        right = vec3(1.0, 0.0, 0.0);
    right = normalize(right);
    float width = length(model[0].xyz), height = length(model[1].xyz);
    vec3 world =
        model[3].xyz + right * (aPosition.x * width) + vec3(0.0, aPosition.y * height, 0.0);
    TexCoord = uvOffset + aTexCoord * uvScale;
    gl_Position = projection * view * vec4(world, 1.0);
}
