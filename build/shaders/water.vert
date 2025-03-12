
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float waveStrength;
uniform float waveSpeed;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Multi-layer wave effect
    vec3 pos = aPos;
    float wave1 = sin(pos.x * 0.1 + time * waveSpeed) * cos(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    float wave2 = sin(pos.x * 0.2 + time * waveSpeed * 1.2) * cos(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    float wave3 = sin(pos.x * 0.05 + time * waveSpeed * 0.7) * cos(pos.z * 0.06 + time * waveSpeed * 0.9) * waveStrength * 0.3;
    
    pos.y = wave1 + wave2 + wave3;
    
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoords = aTexCoords;
    
    // Wave surface normal calculation (based on wave derivatives)
    float dx1 = 0.1 * cos(pos.x * 0.1 + time * waveSpeed) * cos(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    float dz1 = 0.1 * sin(pos.x * 0.1 + time * waveSpeed) * -sin(pos.z * 0.1 + time * waveSpeed * 0.8) * waveStrength;
    
    float dx2 = 0.2 * cos(pos.x * 0.2 + time * waveSpeed * 1.2) * cos(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    float dz2 = 0.15 * sin(pos.x * 0.2 + time * waveSpeed * 1.2) * -sin(pos.z * 0.15 + time * waveSpeed) * waveStrength * 0.5;
    
    vec3 tangent = normalize(vec3(1.0, dx1 + dx2, 0.0));
    vec3 bitangent = normalize(vec3(0.0, dz1 + dz2, 1.0));
    Normal = normalize(cross(tangent, bitangent));
}
