#version 410 core
in vec3 worldPosition, worldNormal, albedo;
out vec4 FragColor;
uniform vec3 lightPosition;
uniform float lightStrength;
void main() {
    vec3 toLight = lightPosition - worldPosition;
    float distance = length(toLight);
    vec3 l = toLight / max(distance, .001);
    vec3 n = normalize(worldNormal);
    if (!gl_FrontFacing)
        n = -n;
    float diffuse = max(dot(n, l), 0.0);
    // A lighter is a small omnidirectional flame, not a focused flashlight beam.
    float falloff =
        (1.0 - smoothstep(3.5, 6.0, distance)) / (1.0 + 1.3 * distance + 1.5 * distance * distance);
    float illumination = lightStrength * diffuse * falloff;
    vec3 surface = albedo;
    if (abs(n.y) > .9) {
        float seam = step(.97, fract(worldPosition.x * 1.8));
        surface *= 1.0 - .25 * seam;
    }
    vec3 lit = surface * (vec3(.0025) + vec3(1.0, .42, .12) * illumination);
    FragColor = vec4(pow(max(lit, vec3(0)), vec3(1.0 / 2.2)), 1);
}
