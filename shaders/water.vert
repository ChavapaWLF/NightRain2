
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
    
    // 多层复杂波浪效果 - 增加更多层次
    vec3 pos = aPos;
    
    // 第一层：大波浪
    float wave1 = sin(pos.x * 0.08 + time * waveSpeed) * cos(pos.z * 0.08 + time * waveSpeed * 0.8) * waveStrength;
    
    // 第二层：中等波浪
    float wave2 = sin(pos.x * 0.15 + time * waveSpeed * 1.3) * cos(pos.z * 0.12 + time * waveSpeed * 1.1) * waveStrength * 0.6;
    
    // 第三层：小波浪
    float wave3 = sin(pos.x * 0.25 + time * waveSpeed * 1.8) * cos(pos.z * 0.22 + time * waveSpeed * 1.5) * waveStrength * 0.3;
    
    // 第四层：微波
    float wave4 = sin(pos.x * 0.4 + time * waveSpeed * 2.2) * cos(pos.z * 0.35 + time * waveSpeed * 2.0) * waveStrength * 0.15;
    
    // 第五层：细微波纹
    float wave5 = sin(pos.x * 0.6 + time * waveSpeed * 2.8) * cos(pos.z * 0.55 + time * waveSpeed * 2.5) * waveStrength * 0.08;
    
    pos.y = wave1 + wave2 + wave3 + wave4 + wave5;
    
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoords = aTexCoords;
    
    // 计算更精确的法线 - 基于所有波浪层的导数
    // X方向导数
    float dx1 = 0.08 * cos(pos.x * 0.08 + time * waveSpeed) * cos(pos.z * 0.08 + time * waveSpeed * 0.8) * waveStrength;
    float dx2 = 0.15 * cos(pos.x * 0.15 + time * waveSpeed * 1.3) * cos(pos.z * 0.12 + time * waveSpeed * 1.1) * waveStrength * 0.6;
    float dx3 = 0.25 * cos(pos.x * 0.25 + time * waveSpeed * 1.8) * cos(pos.z * 0.22 + time * waveSpeed * 1.5) * waveStrength * 0.3;
    float dx4 = 0.4 * cos(pos.x * 0.4 + time * waveSpeed * 2.2) * cos(pos.z * 0.35 + time * waveSpeed * 2.0) * waveStrength * 0.15;
    float dx5 = 0.6 * cos(pos.x * 0.6 + time * waveSpeed * 2.8) * cos(pos.z * 0.55 + time * waveSpeed * 2.5) * waveStrength * 0.08;
    
    // Z方向导数
    float dz1 = 0.08 * sin(pos.x * 0.08 + time * waveSpeed) * -sin(pos.z * 0.08 + time * waveSpeed * 0.8) * waveStrength;
    float dz2 = 0.12 * sin(pos.x * 0.15 + time * waveSpeed * 1.3) * -sin(pos.z * 0.12 + time * waveSpeed * 1.1) * waveStrength * 0.6;
    float dz3 = 0.22 * sin(pos.x * 0.25 + time * waveSpeed * 1.8) * -sin(pos.z * 0.22 + time * waveSpeed * 1.5) * waveStrength * 0.3;
    float dz4 = 0.35 * sin(pos.x * 0.4 + time * waveSpeed * 2.2) * -sin(pos.z * 0.35 + time * waveSpeed * 2.0) * waveStrength * 0.15;
    float dz5 = 0.55 * sin(pos.x * 0.6 + time * waveSpeed * 2.8) * -sin(pos.z * 0.55 + time * waveSpeed * 2.5) * waveStrength * 0.08;
    
    vec3 tangent = normalize(vec3(1.0, dx1 + dx2 + dx3 + dx4 + dx5, 0.0));
    vec3 bitangent = normalize(vec3(0.0, dz1 + dz2 + dz3 + dz4 + dz5, 1.0));
    Normal = normalize(cross(tangent, bitangent));
}
