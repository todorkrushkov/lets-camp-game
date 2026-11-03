#version 410 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D terrainTexture;
void main() {
    FragColor = texture(terrainTexture, TexCoord);
    if (FragColor.a < 0.5)
        discard;
}
