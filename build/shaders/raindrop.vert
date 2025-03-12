
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float raindropSize;
uniform vec3 raindropColor;

out vec3 Color;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = raindropSize / gl_Position.w; // 根据距离调整大小
    Color = raindropColor;
}
