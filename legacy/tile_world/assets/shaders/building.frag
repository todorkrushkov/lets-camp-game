#version 410 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D spriteTexture;
void main() {
    vec4 color = texture(spriteTexture, TexCoord);
    if (color.a < 0.5)
        discard;
    FragColor = vec4(color.rgb, 1.0);
}
