#version 410 core

layout(location = 0) in vec2 aPosition;

uniform vec2 uScreen;

void main() {
    vec2 normalized = aPosition / uScreen;
    vec2 clip = normalized * 2.0 - 1.0;

    gl_Position = vec4(clip.x, -clip.y, 0.0, 1.0);
}
