
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
    TexCoord = uvOffset + aTexCoord * uvScale;

    gl_Position = projection * view * model * vec4(aPosition, 0.0, 1.0);
}
