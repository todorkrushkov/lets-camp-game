
#version 410 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D spriteTexture;
uniform bool occludedPass;

void main() {
    vec4 color = texture(spriteTexture, TexCoord);

    if (color.a < 0.1) {
        discard;
    }

    // Preserve the animated alpha shape when architecture hides the player.
    FragColor = occludedPass ? vec4(0.35, 0.85, 1.0, color.a * 0.65) : color;
}
