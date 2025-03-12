
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

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // 简单的波浪效果
    vec3 pos = aPos;
    pos.y = 0.0 + sin(pos.x * 0.1 + time) * 0.1 + cos(pos.z * 0.1 + time) * 0.1;
    
    gl_Position = projection * view * model * vec4(pos, 1.0);
    TexCoords = aTexCoords;
    
    // 简单的法线计算 (水面向上)
    Normal = vec3(0.0, 1.0, 0.0);
}
