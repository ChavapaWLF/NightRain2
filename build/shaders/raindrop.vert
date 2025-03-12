
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float raindropSize;
uniform vec3 raindropColor;
uniform float brightness;

out vec3 Color;
out float Brightness;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    gl_PointSize = raindropSize / gl_Position.w; // Size adjusted by distance
    Color = raindropColor;
    Brightness = brightness;
}
