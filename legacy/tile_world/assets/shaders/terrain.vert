
#version 410 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 WorldPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 world = model * vec4(aPosition, 1.0);

    WorldPosition = world.xyz;
    TexCoord = aTexCoord;

    gl_Position = projection * view * world;
}
