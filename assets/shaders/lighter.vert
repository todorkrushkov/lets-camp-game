#version 410 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 model, view, projection;
uniform vec2 uvOffset, uvScale;
uniform sampler2D spriteTexture;
void main() {
    vec2 texel = 1.0 / vec2(textureSize(spriteTexture, 0));
    TexCoord = uvOffset + texel * .5 + aTexCoord * (uvScale - texel);
    gl_Position = projection * view * model * vec4(aPosition, 0, 1);
}
