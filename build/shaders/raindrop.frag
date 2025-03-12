
#version 330 core
out vec4 FragColor;

in vec3 Color;

void main() {
    // 创建圆形点
    vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
    if (dot(circCoord, circCoord) > 1.0) {
        discard;
    }
    
    // 添加一些亮度变化
    float brightness = 0.7 + 0.3 * (1.0 - length(circCoord));
    
    FragColor = vec4(Color * brightness, 0.7);
}
